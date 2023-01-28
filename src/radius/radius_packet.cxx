#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "radius_constants.h"
#include "radius_packet.h"

#include "dsSmartException.h"
#include "dsLog.h"
#include "dsApprc.h"

#include "md5.h"

using namespace std;
using namespace libdms5;

/* static factory methods */
RADIUS_Packet *RADIUS_Packet::parse_request(const u_char *buffer, int len, const char *auth_salt) {
  RADIUS_Packet *pkt = new RADIUS_Packet();
  pkt->marshall(buffer, len, auth_salt);
  return pkt;
}

RADIUS_Packet *RADIUS_Packet::create_acct_request(int status_type, const char *auth_info, const char *ipaddr) {
  RADIUS_Packet *request = new RADIUS_Packet();
  request->_code = RAD_ACCT_REQUEST;
  request->_ident = 0x4F;
  request->_length = 20;

  // acct-status-type = 11,12 for webauth
  // acct_session_id = <additional auth information>
  // called_station_id = "webauth"
  // calling_station_id = <IP address of host being authenticated>
  // nas_ip_address = <IP address of the server performing authentication>

  int attribute_idx = 0;
  request->_attributes[attribute_idx++].int_attr(RAD_ATTR_ACCT_STATUS_TYPE, htonl(status_type));
  request->_attributes[attribute_idx++].str_attr(RAD_ATTR_ACCT_SESSION_ID, auth_info, strlen(auth_info));
  request->_attributes[attribute_idx++].str_attr(RAD_ATTR_CALLED_STATION_ID, "dhcpd", sizeof("dhcpd"));
  request->_attributes[attribute_idx++].str_attr(RAD_ATTR_CALLING_STATION_ID, ipaddr, strlen(ipaddr));
  request->_attributes[attribute_idx++].int_attr(RAD_ATTR_NAS_IP_ADDRESS, 0);

  return request;
}

RADIUS_Packet *RADIUS_Packet::create_acct_response(RADIUS_Packet *request) {
  RADIUS_Packet *response = new RADIUS_Packet();
  response->_code = RAD_ACCT_RESPONSE;
  response->_ident = request->_ident;
  response->_length = 20;
  memcpy(response->_auth, request->_auth, 16);
  return response;
}

void RADIUS_Packet::marshall(const u_char *buffer, int pktlength, const char *auth_salt) {
  _code = *buffer;
  _ident = *(buffer + 1);
  //XXX??? _length = htons(*(ushort *)(buffer + 2));
  _length = ntohs(*(ushort *)(buffer + 2));

  // Verify AUTH
  //  Code + Identifier + Length + 16 zero octets + request attributes +
  //  shared secret (where + indicates concatenation).  The 16 octet MD5
  //  hash value is stored in the Authenticator field of the
  //  Accounting-Request packet.

  md5_state_t state;
  md5_byte_t digest[16];

  md5_init(&state);
  // Code + Identifier + Length
  md5_append(&state, (const md5_byte_t *)buffer, 4);
  // auth is all zeroes at this point
  md5_append(&state, (const md5_byte_t *)_auth, 16);
  // taking in account all attributes
  md5_append(&state, (const md5_byte_t *)buffer + 20, _length - 20);

  md5_append(&state, (const md5_byte_t *)auth_salt, strlen(auth_salt));
  md5_finish(&state, digest);

  memcpy(_auth, buffer + 4, 16);

  if (memcmp(_auth, digest, 16) != 0) {
    dsLog::error("Packet authenticator doesn't match");

    if (dsLog::current_verbosity() > 4) {
      dsLog::print_start("auth     {");
      for (int i = 0; i < 16; ++i) { dsLog::print_add("%x", _auth[i]); }
      dsLog::print_eol("}");

      dsLog::print_start("digest   {");
      for (int i = 0; i < 16; ++i) { dsLog::print_add("%x", digest[i]); }
      dsLog::print_eol("}");
    }
  }

  //parse attributes
  int attribute_idx = 0;
  int attribute_offset = 20;

  while(1) {
    if(attribute_offset >= _length) {
      // End of paccket reached
      break;
    }

    if (attribute_idx == MAX_RADIUS_ATTRIBUTES) {
      dsLog::error("Packet contains more than %d attributes, rest will not be read", MAX_RADIUS_ATTRIBUTES);
      break;
    }

    if (*(buffer + attribute_offset) == 0) {
      // Empty attribute, bail out
      break;
    }
    // Attribute layout (vendor specific attributes processed on AttrProcessor level):
    // |type|length|length-2 bytes of data|
    _attributes[attribute_idx]._type = *(buffer + attribute_offset);
    _attributes[attribute_idx]._length = *(buffer + attribute_offset + 1) - 2;
    // _attributes[attribute_idx]._value = new unsigned char[_attributes[attribute_idx]._length];

    // Make sure attribute is NULL terminated, to simplify debugging and logging
    _attributes[attribute_idx]._value = new unsigned char[_attributes[attribute_idx]._length + 1];
    _attributes[attribute_idx]._value[_attributes[attribute_idx]._length] = 0;

    memcpy(_attributes[attribute_idx]._value, buffer + attribute_offset + 2,_attributes[attribute_idx]._length);
    attribute_offset += _attributes[attribute_idx]._length + 2;
    attribute_idx += 1;
  } // while
}

void RADIUS_Packet::unmarshall(u_char *buffer, const char *auth_salt, int *pktlength) {

  // Header lenght
  ushort packet_len = 20;

  // Build packet header
  *(buffer) = _code;
  // memcpy(buffer + 4, auth, 16);

  // attributes
  int attribute_offset = 20;

  for (int i = 0; _attributes[i]._type != 0; ++i) {
    *(buffer + attribute_offset) = _attributes[i]._type;
    *(buffer + attribute_offset + 1) = _attributes[i]._length + 2;
    memcpy(buffer + attribute_offset + 2, _attributes[i]._value, _attributes[i]._length);
    attribute_offset += _attributes[i]._length + 2;
    packet_len += _attributes[i]._length + 2;
  }

  // XXX ??? *(ushort *)(buffer + 2) = ntohs(packet_len);
  *(ushort *)(buffer + 2) = htons(packet_len);

  // Build AUTH
  // Code + Identifier + Length + Request Authenticator field
  // from the Accounting-Request packet being replied to, and the
  // response attributes if any, followed by the shared secret.

  md5_state_t state;
  md5_byte_t digest[16];
  md5_init(&state);

  md5_append(&state, (const md5_byte_t *)buffer, 4);
  md5_append(&state, (const md5_byte_t *)_auth, 16);
  md5_append(&state, (const md5_byte_t *)buffer + 20, packet_len - 20);

  md5_append(&state, (const md5_byte_t *)auth_salt, strlen(auth_salt));
  md5_finish(&state, digest);

  // Replace auth with new one
  memcpy(buffer+4, digest, 16);

  *pktlength = packet_len;
}

const char * radius_pkt_names[] = {
  "Unknown", "Access-Request", "Access-Accept", "Access-Reject",
  "Accounting-Request", "Accounting-Response"
};

const char *RADIUS_Packet::code_name(ushort code) {
  if (code > (sizeof(radius_pkt_names)/ sizeof(char*)) - 1) {
     return "UNKNOWN";
  }

  return radius_pkt_names[code];
}

void RADIUS_Packet::dump() {
  dsLog::print("--------------DUMP RADIUS PACKET-------------");
  dsLog::print("code   = %s (%d)", code_name(_code), _code);
  dsLog::print("ident  = %d", _ident);
  dsLog::print("length = %d", _length);
  dsLog::print_start("auth     {");
  for (int i = 0; i < 16; ++i) { dsLog::print_add("%x", _auth[i]); }
  dsLog::print_eol("}");
  dsLog::print("--------");

  for (int i = 0; _attributes[i]._type != 0; ++i) {
    _attributes[i].dump();
  }
  dsLog::print("---------------------------------------------");
}


const char *radius_attr_names[] = {
  "Unknown",
  "User-Name",          // 1
  "User-Password",
  "CHAP-Password",
  "NAS-IP-Address",
  "NAS-Port",
  "Service-Type",
  "Framed-Protocol",
  "Framed-IP-Address",  // 8
  "Framed-IP-Netmask",
  "Framed-Routing",
  "Filter-Id",
  "Framed-MTU",
  "Framed-Compression",
  "Login-IP-Host",
  "Login-Service",
  "Login-TCP-Port",     // 16
  "User-Old-Password?",
  "Reply-Message",
  "Callback-Number",
  "Callback-Id",
  "Expiration?",
  "Framed-Route",
  "Framed-IPX-Network",
  "State",             // 24
  "Class",
  "Vendor-Specific",
  "Session-Timeout",
  "Idle-Timeout",
  "Termination-Action",
  "Called-Station-Id",
  "Calling-Station-Id",
  "NAS-Identifier",     // 32
  "Proxy-State",
  "Login-LAT-Service?",
  "Login-LAT-Node?",
  "Login-LAT-Group?",
  "Framed-AppleTalk-Link?",
  "Framed-AppleTalk-Network?",
  "Framed-AppleTalk-Zone?",
  "Acct-Status-Type",   // 40
  "Acct-Delay-Time",
  "Acct-Input-Octets",
  "Acct-Output-Octets",
  "Acct-Session-Id",
  "Acct-Authentic",
  "Acct-Session-Time",
  "Acct-Input-Packets",
  "Acct-Output-Packets", // 48
  "Acct-Terminate-Cause",
  "Acct-Multi-Session-Id?",
  "Acct-Link-Count?",
  "Acct-Input-Gigawords",
  "Acct-Output-Gigawords",
  "Unknown?"             // 54
  "Event-Timestamp",
  "Egress-VLANID?",
  "Ingress-Filters?",
  "Egress-VLAN-Name?",
  "User-Priority-Table?",
  "CHAP-Challenge",
  "NAS-Port-Type",
  "Port-Limit",
  "Login-LAT-Port?",
  "Tunnel-Type?",      // 64
  "Tunnel-Medium-Type?",
  "Tunnel-Client-Endpoint?",
  "Tunnel-Server-Endpoint?",
  "Acct-Tunnel-Connection?",
  "Tunnel-Password?",
  "ARAP-Password",
  "ARAP-Features",
  "ARAP-Zone-Access", // 72
  "ARAP-Security",
  "ARAP-Security-Data",
  "Password-Retry",
  "Prompt",
  "Connect-Info",
  "Configuration-Token",
  "EAP-Message",
  "Message-Authenticator", // 80
  "Tunnel-Private-Group-ID?",
  "Tunnel-Assignment-ID?",
  "Tunnel-Preference?",
  "ARAP-Challenge-Response",
  "Acct-Interim-Interval?",
  "Acct-Tunnel-Packets-Lost?",
  "NAS-Port-Id",
  "Framed-Pool",  // 88
  "Cargeable-User-Identity",
  /* Constants below is not verified */
  "Tunnel-Client-Auth-ID?",
  "Tunnel-Server-Auth-ID?",
  "NAS-Filter-Rule?",
  "Originating-Line-Info?",
  "NAS-IPv6-Address?",
  "Framed-Interface-Id?",
  "Framed-IPv6-Prefix?",
  "Login-IPv6-Host?",
  "Framed-IPv6-Route?",
  "Framed-IPv6-Pool?",
  "Error-Cause Attribute?",
  "EAP-Key-Name?",
  "Digest-Response?",
  "Digest-Realm?",
  "Digest-Nonce?",
  "Digest-Response-Auth?",
  "Digest-Nextnonce?",
  "Digest-Method?",
  "Digest-URI?",
  "Digest-Qop?",
  "Digest-Algorithm?",
  "Digest-Entity-Body-Hash?",
  "Digest-CNonce?",
  "Digest-Nonce-Count?",
  "Digest-Username?",
  "Digest-Opaque?",
  "Digest-Auth-Param?",
  "Digest-AKA-Auts?",
  "Digest-Domain?",
  "Digest-Stale?",
  "Digest-HA1?",
  "SIP-AOR?",
  "Delegated-IPv6-Prefix?",
  "MIP6-Feature-Vector?",
  "MIP6-Home-Link-Prefix?",
  "Operator-Name?",
  "Location-Information?",
  "Location-Data?",
  "Basic-Location-Policy-Rules?",
  "Extended-Location-Policy-Rules?",
  "Location-Capable?",
  "Requested-Location-Info?",
  "Framed-Management-Protocol?",
  "Management-Transport-Protection?",
  "Management-Policy-Id?",
  "Management-Privilege-Level?",
  "PKM-SS-Cert?",
  "PKM-CA-Cert?",
  "PKM-Config-Settings?",
  "PKM-Cryptosuite-List?",
  "PKM-SAID?",
  "PKM-SA-Descriptor?",
  "PKM-Auth-Key?",
  "DS-Lite-Tunnel-Name?",
  "Mobile-Node-Identifier?",
  "Service-Selection?",
  "PMIP6-Home-LMA-IPv6-Address?",
  "PMIP6-Visited-LMA-IPv6-Address?",
  "PMIP6-Home-LMA-IPv4-Address?",
  "PMIP6-Visited-LMA-IPv4-Address?",
  "PMIP6-Home-HN-Prefix?",
  "PMIP6-Visited-HN-Prefix?",
  "PMIP6-Home-Interface-ID?",
  "PMIP6-Visited-Interface-ID?",
  "PMIP6-Home-IPv4-HoA?",
  "PMIP6-Visited-IPv4-HoA?",
  "PMIP6-Home-DHCP4-Server-Address?",
  "PMIP6-Visited-DHCP4-Server-Address?",
  "PMIP6-Home-DHCP6-Server-Address?",
  "PMIP6-Visited-DHCP6-Server-Address?",
  "PMIP6-Home-IPv4-Gateway?",
  "PMIP6-Visited-IPv4-Gateway?",
  "EAP-Lower-Layer?",
  "GSS-Acceptor-Service-Name?",
  "GSS-Acceptor-Host-Name?",
  "GSS-Acceptor-Service-Specifics?",
  "GSS-Acceptor-Realm-Name?",
  "Framed-IPv6-Address?",
  "DNS-Server-IPv6-Address?",
  "Route-IPv6-Information?",
  "Delegated-IPv6-Prefix-Pool?",
  "Stateful-IPv6-Address-Pool?",
  "IPv6-6rd-Configuration?",
  "Allowed-Called-Station-Id?",
  "EAP-Peer-Id?",
  "EAP-Server-Id?",
  "Mobility-Domain-Id?",
  "Preauth-Timeout?",
  "Network-Id-Name?",
  "EAPoL-Announcement?",
  "WLAN-HESSID?",
  "WLAN-Venue-Info?",
  "WLAN-Venue-Language?",
  "WLAN-Venue-Name?",
  "WLAN-Reason-Code?",
  "WLAN-Pairwise-Cipher?",
  "WLAN-Group-Cipher?",
  "WLAN-AKM-Suite?",
  "WLAN-Group-Mgmt-Cipher?",
  "WLAN-RF-Band?" };

const char *RADIUS_Attribute::attr_name(ushort type) {
  if (type > (sizeof(radius_attr_names)/ sizeof(char*)) - 1) {
     return "UNKNOWN";
  }

  return radius_attr_names[type];
}

void RADIUS_Attribute::dump() {

#define PRINT_VALUE(fmt) \
      dsLog::print_start("       %s(%d,%d) {", attr_name(_type), _type, _length); \
      for (int i = 0; i < _length; ++i) { dsLog::print_add(fmt, _value[i]); } \
      dsLog::print_eol("}");

  switch(_type) {
    case RAD_ATTR_USER_NAME:
      PRINT_VALUE("%c")
      break;
    default:
      PRINT_VALUE("%x ")
  }

#undef PRINT_VALUE
}
