#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dhcp_constants.h"
#include "dhcp_packet.h"

#include "subnets.h"

#include "dsSmartException.h"
#include "dsLog.h"
#include "dsApprc.h"

using namespace std;
using namespace libdms5;

uint8_t DHCP_MAGIC_COOKIE[4] = {0x63, 0x82, 0x53, 0x63};

/* static factory methods */

DHCP_Packet *DHCP_Packet::parse_request(const u_char *buffer, int len) {
  DHCP_Packet *pkt = new DHCP_Packet();
  pkt->marshall(buffer, len);
  return pkt;
}

DHCP_Packet *DHCP_Packet::create_dhcp_offer(DHCP_Packet *request, const IPAddress& yiaddr) {

  DHCP_Packet *response = new DHCP_Packet();

  response->op = BOOT_REPLY;
  response->htype = request->htype;
  response->hlen = request->hlen;
  response->hops = 0;
  response->xid = request->xid;
  response->secs = 0;
  response->ciaddr = 0;
  response->yiaddr = yiaddr.u_addr;
  response->siaddr = 0;  // address of next bootstrap server e.g. tftp
  response->flags = request->flags;
  response->giaddr = request->giaddr;
  memcpy(response->chaddr, request->chaddr, 16);
  memset(response->sname,0, sizeof(response->sname)); //server hostname
  memset(response->file,0, sizeof(response->file)); //server hostname

  // global couldn't be NULL
  Subnet *global = Subnets::globalnet();

  Subnet *mysub = Subnets::mysubnet(yiaddr);
  // IP have to be verified on allocator side
  // but defend to bad allocator behaviour here
  if (mysub == NULL) {
    throw DHCP_PacketException("ip '%s' is not within allowed range",yiaddr.s_addr);
  }

  //options
  int option_idx = 0;

  response->_options[option_idx].set_code(DHO_DHCP_MESSAGE_TYPE);
  response->_options[option_idx].set_value(DHCP_OFFER);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DHCP_SERVER_IDENTIFIER);
  response->_options[option_idx].set_value(global->u_addr());
  option_idx += 1;

  //TODO: Allow per subnet lease and renew_time time
  // Lease time should be propagated to IP allocator and stored to database
  response->_options[option_idx].set_code(DHO_DHCP_LEASE_TIME);
  response->_options[option_idx].set_value(htonl(global->lease_time));
  option_idx += 1;

  // Optional options
  response->_options[option_idx].set_code(DHO_DHCP_RENEWAL_TIME);
  response->_options[option_idx].set_value(htonl(global->renew_time));
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DOMAIN_NAME);
  response->_options[option_idx].set_value(global->domain_name, strlen(global->domain_name));
  option_idx += 1;

  // Subnet mask and router is not required for offer but desired
  response->_options[option_idx].set_code(DHO_SUBNET);
  response->_options[option_idx].set_value(mysub->netmask.u_addr);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_ROUTERS);
  response->_options[option_idx].set_value(mysub->gateway.u_addr);
  option_idx += 1;

  // Some clients await DNS in offer messages
  // DNS handled specially
  unsigned char *dns = new unsigned char[8];
  *(uint32_t *) dns = mysub->nameserver1.u_addr;
  // Nameserver2 is optional
  if (mysub->nameserver2.u_addr != INADDR_NONE) {
    *(uint32_t *) (dns+4) = mysub->nameserver2.u_addr;
  }

  response->_options[option_idx].set_code(DHO_DOMAIN_NAME_SERVERS);
  response->_options[option_idx].value = dns;
  response->_options[option_idx].length = 8;
  option_idx += 1;

  // Paranoya, actually not necesary
  response->_options[option_idx].set_code(DHO_END);

  return response;
}

DHCP_Packet *DHCP_Packet::create_dhcp_ack(DHCP_Packet *request, const IPAddress& yiaddr) {

  DHCP_Packet *response = new DHCP_Packet();

  response->op = BOOT_REPLY;
  response->htype = request->htype;
  response->hlen = request->hlen;
  response->hops = 0;
  response->xid = request->xid;
  response->secs = 0;
  response->ciaddr = 0;
  response->yiaddr = yiaddr.u_addr;
  response->siaddr = 0;  // address of next bootstrap server e.g. tftp
  response->flags = request->flags;
  response->giaddr = request->giaddr;
  memcpy(response->chaddr, request->chaddr, 16);
  memset(response->sname,0, sizeof(response->sname)); //server hostname
  memset(response->file,0, sizeof(response->file)); //server hostname

  // global couldn't be NULL
  Subnet *global = Subnets::globalnet();

  Subnet *mysub = Subnets::mysubnet(yiaddr);
  // IP have to be verified on allocator side
  // but defend to bad allocator behaviour here
  if (mysub == NULL) {
    dsLog::error("DHCP_ACK ip '%s' is not within allowed range", yiaddr.s_addr);
    delete response;
    return NULL;
  }

  //options
  int option_idx = 0;

  response->_options[option_idx].set_code(DHO_DHCP_MESSAGE_TYPE);
  response->_options[option_idx].set_value(DHCP_ACK);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DHCP_SERVER_IDENTIFIER);
  response->_options[option_idx].set_value(global->u_addr());
  option_idx += 1;

  //TODO: Allow per subnet lease and renew_time time
  // Lease time should be propagated to IP allocator and stored to database
  response->_options[option_idx].set_code(DHO_DHCP_LEASE_TIME);
  response->_options[option_idx].set_value(htonl(global->lease_time));
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DHCP_RENEWAL_TIME);
  response->_options[option_idx].set_value(htonl(global->renew_time));
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DOMAIN_NAME);
  response->_options[option_idx].set_value(global->domain_name, strlen(global->domain_name));
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_ROUTERS);
  response->_options[option_idx].set_value(mysub->gateway.u_addr);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_SUBNET);
  response->_options[option_idx].set_value(mysub->netmask.u_addr);
  option_idx += 1;

  // dns handled specially
  unsigned char *dns = new unsigned char[8];
  *(uint32_t *) dns = mysub->nameserver1.u_addr;
  // Nameserver2 is optional
  if (mysub->nameserver2.u_addr != INADDR_NONE) {
    *(uint32_t *) (dns+4) = mysub->nameserver2.u_addr;
  }

  response->_options[option_idx].set_code(DHO_DOMAIN_NAME_SERVERS);
  response->_options[option_idx].value = dns;
  response->_options[option_idx].length = 8;
  option_idx += 1;

  // Paranoya, actually not necesary
  response->_options[option_idx].set_code(DHO_END);

  return response;
}

// This packet is constructed in reply to DHCP_INFORM message, unfortunately standard authors
// didn't allocate a separate type for the packet.

DHCP_Packet *DHCP_Packet::create_dhcp_ack2(DHCP_Packet *request, const IPAddress& yiaddr) {

  DHCP_Packet *response = new DHCP_Packet();

  response->op = BOOT_REPLY;
  response->htype = request->htype;
  response->hlen = request->hlen;
  response->hops = 0;
  response->xid = request->xid;
  response->secs = 0;
  response->ciaddr = request->ciaddr;
  response->yiaddr = 0;
  response->siaddr = 0;  // address of next bootstrap server e.g. tftp
  response->flags = request->flags;
  response->giaddr = request->giaddr;
  memcpy(response->chaddr, request->chaddr, 16);
  memset(response->sname,0, sizeof(response->sname)); //server hostname
  memset(response->file,0, sizeof(response->file)); //server hostname

  // global couldn't be NULL
  Subnet *global = Subnets::globalnet();

  Subnet *mysub = Subnets::mysubnet(yiaddr);
  // We must not verify IP address on DHCP_INFORM,
  // So if IP we are asked for is not within one of configured ranges
  // we just don't answer to INFORM, allow client to re-negotiate config

  if (mysub == NULL) {
    dsLog::error("DHCP_ACK2 ip '%s' is not within allowed range", yiaddr.s_addr);
    delete response;
    return NULL;
  }

  //options
  int option_idx = 0;

  response->_options[option_idx].set_code(DHO_DHCP_MESSAGE_TYPE);
  response->_options[option_idx].set_value(DHCP_ACK);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DHCP_SERVER_IDENTIFIER);
  response->_options[option_idx].set_value(global->u_addr());
  option_idx += 1;

  // According to RFC2131 we must not send LEASE_TIME in response to DHCP_INFORM request

  response->_options[option_idx].set_code(DHO_DOMAIN_NAME);
  response->_options[option_idx].set_value(global->domain_name, strlen(global->domain_name));
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_ROUTERS);
  response->_options[option_idx].set_value(mysub->gateway.u_addr);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_SUBNET);
  response->_options[option_idx].set_value(mysub->netmask.u_addr);
  option_idx += 1;

  // dns handled specially
  unsigned char *dns = new unsigned char[8];
  *(uint32_t *) dns = mysub->nameserver1.u_addr;
  // Nameserver2 is optional
  if (mysub->nameserver2.u_addr != INADDR_NONE) {
    *(uint32_t *) (dns+4) = mysub->nameserver2.u_addr;
  }

  response->_options[option_idx].set_code(DHO_DOMAIN_NAME_SERVERS);
  response->_options[option_idx].value = dns;
  response->_options[option_idx].length = 8;
  option_idx += 1;

  // Paranoya, actually not necesary
  response->_options[option_idx].set_code(DHO_END);

  return response;
}

DHCP_Packet *DHCP_Packet::create_dhcp_nak(DHCP_Packet *request, const IPAddress& yiaddr /* unused */) {

  DHCP_Packet *response = new DHCP_Packet();

  response->op = BOOT_REPLY;
  response->htype = request->htype;
  response->hlen = request->hlen;
  response->hops = 0;
  response->xid = request->xid;
  response->secs = 0;
  response->ciaddr = 0;
  response->yiaddr = 0;
  response->siaddr = 0;  // address of next bootstrap server e.g. tftp
  response->flags = request->flags;
  response->giaddr = request->giaddr;
  memcpy(response->chaddr, request->chaddr, 16);
  memset(response->sname,0, sizeof(response->sname)); //server hostname
  memset(response->file,0, sizeof(response->file)); //server hostname

  // global couldn't be NULL
  Subnet *global = Subnets::globalnet();

  //options
  int option_idx = 0;

  response->_options[option_idx].set_code(DHO_DHCP_MESSAGE_TYPE);
  response->_options[option_idx].set_value(DHCP_NAK);
  option_idx += 1;

  response->_options[option_idx].set_code(DHO_DHCP_SERVER_IDENTIFIER);
  response->_options[option_idx].set_value(global->u_addr());
  option_idx += 1;

  // Paranoya, actually not necesary
  response->_options[option_idx].set_code(DHO_END);

  return response;
}

bool DHCP_Packet::check_magic_cookie(const u_char *buffer, int len) {
  //check DHCP magic cookie
  if(len < 240 || memcmp(DHCP_MAGIC_COOKIE, buffer + 236, 4) != 0) {
    return false;
  }

  return true;
}

void DHCP_Packet::marshall(const u_char *buffer, int pktlength) {

  //Pranoya, it have to be done on upper level
  if (! DHCP_Packet::check_magic_cookie(buffer, pktlength)) {
    throw DHCP_PacketException("Invalid dhcp magic cookie");
  }

  // Parse packet header
  op = *buffer;
  htype = *(buffer + 1);
  hlen = *(buffer + 2);
  hops = *(buffer + 3);
  xid = *(uint32_t*)(buffer + 4);
  secs = *(short*)(buffer + 8);
  flags = *(short*)(buffer + 10);
  ciaddr = *(uint32_t*)(buffer + 12);
  yiaddr = *(uint32_t*)(buffer + 16);
  siaddr = *(uint32_t*)(buffer + 20);
  giaddr = *(uint32_t*)(buffer + 24);

  memcpy(chaddr, buffer + 28, 16);
  memcpy(sname, buffer + 44, 64);
  memcpy(file, buffer + 108, 128);

 //parse options
  int option_idx = 0;
  int option_offset = 240; //236 + 4

  while(1) {
    if(option_offset > pktlength - 1) {
      dsLog::error("The options length is more than packet length, but no end mark.");
      break;
    }

    if (option_idx == MAX_DHCP_OPTIONS) {
      dsLog::error("Packet contains more than %d options, rest will not be read.", MAX_DHCP_OPTIONS);
      break;
    }

    // Packet can contain zero bytes between options, skip it
    if(*(buffer + option_offset) == DHO_PAD) {
      option_offset += 1;
      continue;
    }

    // Option layout:
    // |code|len|........|
    _options[option_idx].code = *(buffer + option_offset);
    if (_options[option_idx].code == DHO_END) {
      break;
    }

    _options[option_idx].length = *(buffer + option_offset + 1);
    _options[option_idx].value = new unsigned char[_options[option_idx].length];
    memcpy(_options[option_idx].value, buffer + option_offset + 2,_options[option_idx].length);

    option_offset += _options[option_idx].length + 2;
    option_idx += 1;
  } // while

  // Make sure we have DHO_END,
  // otherwise we can crash on incorrect packet
  _options[option_idx].code = DHO_END;

  // Paddinf will not be read
}

void DHCP_Packet::unmarshall(u_char *buffer, int *pktlength) {

    //calculate the total size of the packet
    //static part
    int packet_len = BOOTP_ABSOLUTE_MIN_LEN;
    //magic cookie
    packet_len += sizeof(DHCP_MAGIC_COOKIE);
    //options
    for (int i = 0; _options[i].code != DHO_END; ++i) {
      packet_len += (_options[i].length + 2);
    }

    //end option
    packet_len++;

    //calculate padding length

    // A number of dhcp servers and relays check minimal dhcp
    // packet size and drops anything less than 300 octets long as per RFC 1542.

    int padding_len = 0;
    if(packet_len < BOOTP_ABSOLUTE_MIN_LEN + DHCP_VEND_SIZE) {
      padding_len = DHCP_VEND_SIZE + BOOTP_ABSOLUTE_MIN_LEN - packet_len;
      packet_len = DHCP_VEND_SIZE + BOOTP_ABSOLUTE_MIN_LEN;
    }

    // Build packet header
    *(buffer) = op;
    *(buffer + 1) = htype;
    *(buffer + 2) = hlen;
    *(buffer + 3) = hops;
    *(uint32_t*)(buffer + 4) = xid;
    *(short*)(buffer + 8) = secs;
    *(short*)(buffer + 10) = flags;
    *(uint32_t*)(buffer + 12) = ciaddr;
    *(uint32_t*)(buffer + 16) = yiaddr;
    *(uint32_t*)(buffer + 20) = siaddr;
    *(uint32_t*)(buffer + 24) = giaddr;

    memcpy(buffer + 28, chaddr, 16);
    memcpy(buffer + 44, sname, 64);
    memcpy(buffer + 108, file, 128);
    memcpy(buffer + 236, DHCP_MAGIC_COOKIE, 4);

    // options
    int option_offset = 240;

    for (int i = 0; _options[i].code != DHO_END; ++i) {
      *(buffer + option_offset) = _options[i].code;
      *(buffer + option_offset + 1) = _options[i].length;
      memcpy(buffer + option_offset + 2, _options[i].value, _options[i].length);
      option_offset += _options[i].length + 2;
    }

    *(buffer + option_offset) = DHO_END;
    option_offset++;

    if(padding_len > 0) {
      memset(buffer + option_offset, 0, padding_len);
    }

    *pktlength = packet_len;
}

int DHCP_Packet::get_message_type() {
  for (int i = 0; _options[i].code != DHO_END; ++i) {
    if(_options[i].code == DHO_DHCP_MESSAGE_TYPE) {
      return *(_options[i].value) & 0xFF;
    }
  }

  return -1;
}

IPAddress DHCP_Packet::get_requested_address() {
  // 1. Check caddr if it's not 0.0.0.0 return caddr
  if (ciaddr != 0) {
    return IPAddress(ciaddr);
  }

  // 2. Look for dhcp option
  for (int i = 0; _options[i].code != DHO_END; ++i) {
    if(_options[i].code == DHO_DHCP_REQUESTED_ADDRESS) {
      return IPAddress(*(uint32_t *)(_options[i].value));
    }
  }

  // 3. Return INADDR_NONE
  return IPAddress();
}



void DHCP_Packet::dump() {
  dsLogScope scope("dhcp");

  char mac_str[20];
  char ip_str[20];

  mac_u2s(*(uint64_t *)chaddr, mac_str);

  dsLogPrefix prefix(mac_str);

  dsLog::print("--------------DUMP DHCP PACKET-------------");
  dsLog::print("op     = %d", op);
  dsLog::print("htype  = %d", htype);
  dsLog::print("hlen   = %d", hlen);
  dsLog::print("hops   = %d", hops);
  dsLog::print("xid    = %x", xid);
  dsLog::print("secs   = %x", secs);
  dsLog::print("flags  = %x", flags);

  dsLog::print("ciaddr = %s", ip_u2s(ciaddr, ip_str));
  dsLog::print("yiaddr = %s", ip_u2s(yiaddr, ip_str));
  dsLog::print("siaddr = %s", ip_u2s(siaddr, ip_str));
  dsLog::print("giaddr = %s", ip_u2s(giaddr, ip_str));

  dsLog::print("chaddr = %s", mac_str);
  dsLog::print("sname  = %s", sname);
  dsLog::print("file   = %s", file);
  dsLog::print("---------------------------------------------");

  for (int i = 0; _options[i].code != DHO_END; ++i) {
    _options[i].dump();
  }
}

/* constants for pretty printing of packet */

const char *dhcp_dho_names[] = {
  "DHO_PAD", "DHO_SUBNET", "DHO_TIME_OFFSET", "DHO_ROUTERS", "DHO_TIME_SERVERS",
  "DHO_NAME_SERVERS", "DHO_DOMAIN_NAME_SERVERS", "DHO_LOG_SERVER", "DHO_COOKIE_SERVERS",
  "DHO_LPR_SERVERS", "DHO_IMPRESS_SERVER", "DHO_RESOURCE_LOCATION_SERVERS", "DHO_HOST_NAME",
  "DHO_BOOT_SIZE", "DHO_MERIT_DUMP", "DHO_DOMAIN_NAME", "DHO_SWAP_SERVER", "DHO_ROOT_PATH",
  "DHO_EXTENSIONS_PATH", "DHO_IP_FORWARDING", "DHO_NON_LOCAL_SOURCE_ROUTING", "DHO_POLICY_FILTER",
  "DHO_MAX_DGRAM_REASSEMBLY", "DHO_DEFAULT_IP_TTL", "DHO_PATH_MTU_AGING_TIMEOUT", "DHO_PATH_MTU_PLATEAU_TABLE",
  "DHO_INTERFACE_MTU", "DHO_ALL_SUBNETS_LOCAL", "DHO_BROADCAST_ADDRESS", "DHO_PERFORM_MASK_DISCOVERY",
  "DHO_MASK_SUPPLIER", "DHO_ROUTER_DISCOVERY", "DHO_ROUTER_SOLICITATION_ADDRESS", "DHO_STATIC_ROUTES",
  "DHO_TRAILER_ENCAPSULATION", "DHO_ARP_CACHE_TIMEOUT", "DHO_IEEE802_3_ENCAPSULATION", "DHO_DEFAULT_TCP_TTL",
  "DHO_TCP_KEEPALIVE_INTERVAL", "DHO_TCP_KEEPALIVE_GARBAGE", "DHO_RESERVED", "DHO_NIS_SERVERS", "DHO_NTP_SERVERS",
  "DHO_VENDOR_ENCAPSULATED_OPTIONS", "DHO_NETBIOS_NAME_SERVERS", "DHO_NETBIOS_DD_SERVER", "DHO_NETBIOS_NODE_TYPE",
  "DHO_NETBIOS_SCOPE", "DHO_FONT_SERVERS", "DHO_X_DISPLAY_MANAGER", "DHO_DHCP_REQUESTED_ADDRESS",
  "DHO_DHCP_LEASE_TIME", "DHO_DHCP_OPTION_OVERLOAD", "DHO_DHCP_MESSAGE_TYPE", "DHO_DHCP_SERVER_IDENTIFIER",
  "DHO_DHCP_PARAMETER_REQUEST_LIST", "DHO_DHCP_MESSAGE", "DHO_DHCP_MAX_MESSAGE_SIZE", "DHO_DHCP_RENEWAL_TIME",
  "DHO_DHCP_REBINDING_TIME", "DHO_VENDOR_CLASS_IDENTIFIER", "DHO_DHCP_CLIENT_IDENTIFIER", "DHO_NWIP_DOMAIN_NAME",
  "DHO_NWIP_SUBOPTIONS", "DHO_NISPLUS_DOMAIN", "DHO_NISPLUS_SERVER", "DHO_TFTP_SERVER", "DHO_BOOTFILE",
  "DHO_MOBILE_IP_HOME_AGENT", "DHO_SMTP_SERVER", "DHO_POP3_SERVER", "DHO_NNTP_SERVER", "DHO_WWW_SERVER",
  "DHO_FINGER_SERVER", "DHO_IRC_SERVER",  "DHO_STREETTALK_SERVER", "DHO_STDA_SERVER", "DHO_USER_CLASS",
  "DHO_FQDN", "DHO_DHCP_AGENT_OPTIONS", "DHO_NDS_SERVERS", "DHO_NDS_TREE_NAME", "DHO_NDS_CONTEXT",
  "DHO_CLIENT_LAST_TRANSACTION_TIME", "DHO_ASSOCIATED_IP", "DHO_USER_AUTHENTICATION_PROTOCOL",
  "DHO_AUTO_CONFIGURE", "DHO_NAME_SERVICE_SEARCH", "DHO_SUBNET_SELECTION", "DHO_DOMAIN_SEARCH",
  "DHO_CLASSLESS_ROUTE"
  };

const char *dhcp_msgtype_names[] = {
    "DHCP_NONE", "DHCP_DISCOVER", "DHCP_OFFER", "DHCP_REQUEST",
    "DHCP_DECLINE", "DHCP_ACK", "DHCP_NAK", "DHCP_RELEASE",
    "DHCP_INFORM", "DHCP_FORCE_RENEW", "DHCP_LEASE_QUERY", "DHCP_LEASE_UNASSIGNED",
    "DHCP_LEASE_UNKNOWN", "DHCP_LEASE_ACTIVE"
  };


const char *DHCP_Option::msgtype_name(int mt) {
  if (mt > (sizeof(dhcp_msgtype_names)/ sizeof(char*)) - 1) {
    return "INTERNAL_UNKNOWN";
  }

  return dhcp_msgtype_names[mt];
}

const char *DHCP_Option::dho_name(int code) {
  if (code > (sizeof(dhcp_dho_names)/ sizeof(char*)) - 1) {
    return "INTERNAL_UNKNOWN";
  }

  return dhcp_dho_names[code];
}


void DHCP_Option::dump() {

#define PRINT_VALUE(fmt) \
      dsLog::print_start("       %s(%d) {", dho_name(code), code); \
      for (int i = 0; i < length; ++i) { dsLog::print_add(fmt, value[i]); } \
      dsLog::print_eol("}");


  char mac_str[20];
  char ip_str[20];

  switch(code) {
    case  DHO_DHCP_MESSAGE_TYPE: {
      int mt = (*value & 0xFF);
      dsLog::print("       %s(%d) *** %s", dho_name(code), code, msgtype_name(mt));
      break;
    }
    case DHO_DHCP_CLIENT_IDENTIFIER:
      // Every request sent from a DHCP client to DHCP servers contains a hardware type
      // and a client hardware address.
      // For Ethernet and 802.11 wireless clients, the hardware type is always 01.
      dsLog::print("       %s(%d) %x %s", dho_name(code), code, *(value) & 0xFF, mac_u2s(*(uint64_t *)(value+1), mac_str));
      break;
    case DHO_DHCP_SERVER_IDENTIFIER:
      dsLog::print("       %s(%d) %x %s", dho_name(code), code, *(uint32_t *)value, ip_u2s(*(uint32_t *)value, ip_str));
      break;
     case DHO_DHCP_PARAMETER_REQUEST_LIST:
      PRINT_VALUE("%d ");
      break;
    case   DHO_VENDOR_CLASS_IDENTIFIER:
    case   DHO_HOST_NAME:
      PRINT_VALUE("%c");
      break;
    case   DHO_DHCP_REQUESTED_ADDRESS:
      dsLog::print("       %s(%d) %s", dho_name(code), code, ip_u2s(*(uint32_t *)value, ip_str));
      break;
    default:
      PRINT_VALUE("%x ")
  }

#undef PRINT_VALUE
}
