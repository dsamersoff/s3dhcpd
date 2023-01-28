#ifndef _DHCP_PACKET_H_
#define _DHCP_PACKET_H_

#include "dsSmartException.h"

#include "subnets.h"

DECLARE_EXCEPTION(DHCP_Packet);

#define MAX_DHCP_OPTIONS 32

struct DHCP_Option {
    unsigned char        code;
    unsigned char      length;
    unsigned char      *value;

    DHCP_Option() {
      code = (unsigned char)-1; //DHO_END
      value = 0, length = 0;
    }

    static const char *dho_name(int code);
    static const char *msgtype_name(int mt);

    void set_code(unsigned char p_code) {
      code = p_code;
    }

    void set_value(unsigned char p_val) {
      value = new unsigned char[1];
      *value = p_val;
      length = 1;
    }

    void set_value(uint32_t p_val) {
      value = new unsigned char[4];
      *(uint32_t *)value = p_val;
      length = 4;
    }

    void set_value(const char *p_val, int p_len) {
      value = new unsigned char[p_len];
      memcpy(value,p_val,p_len);
      length = p_len;
    }

    ~DHCP_Option() {
      delete[] value;
    }

    void dump();
};

class DHCP_Packet {
  /* dhcp packet header */
    char op;
    char htype;
    char hlen;
    char hops;
    uint32_t xid;
    short secs;
    short flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    unsigned char chaddr[16];
    char sname[64];
    char file[128];
    // options
    DHCP_Option _options[MAX_DHCP_OPTIONS];

    // New packet have to be created by factory methods only
    DHCP_Packet() {
      memset(this, 0, sizeof(*this)); //XXX
    }

public:

    // Get packet from buffer
    void marshall(const u_char *buffer, int len);

    // Assemble packet back to buffer, return actual packet lengh
    void unmarshall(u_char *buffer, int *len);

    static bool check_magic_cookie(const u_char *buffer, int len);

    // create packet from network received buffer
    static DHCP_Packet *parse_request(const u_char *buffer, int len);

    // create packet
    static DHCP_Packet *create_dhcp_offer(DHCP_Packet *request, const IPAddress& yiaddr);
    static DHCP_Packet *create_dhcp_ack(DHCP_Packet *request, const IPAddress& yiaddr);
    // In reply to DHCPINFORM. The same as ACK but with minor difference.
    // TODO: Consider merging two functions
    static DHCP_Packet *create_dhcp_ack2(DHCP_Packet *request, const IPAddress& yiaddr);
    static DHCP_Packet *create_dhcp_nak(DHCP_Packet *request, const IPAddress& yiaddr);

    // Return value of ciaddr packet field should be 0 for all except DHCPINFORM
    uint32_t get_ciaddr() { return ciaddr; }

    // find option 53 and return it's value
    int get_message_type();

    IPAddress get_requested_address();

    // dump packet in human readable form
    const char *msgtype_name() {
      unsigned char mt = get_message_type();
      return DHCP_Option::msgtype_name(mt);
    }

    void dump();

    // uint64_t mac(){ return *(uint64_t *) chaddr; }
    // uint32_t ip(){ return ciaddr; }
    MACAddress mac(){ return MACAddress(*(uint64_t *) chaddr); }
    IPAddress ip(){ return IPAddress(ciaddr); }
};

#endif
