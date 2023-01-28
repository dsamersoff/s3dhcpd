#ifndef _RADIUS_PACKET_H_
#define _RADIUS_PACKET_H_

#include <stdint.h>
#include <arpa/inet.h>

#include "dsSmartException.h"

#include "radius_constants.h"

DECLARE_EXCEPTION(Packet);

#define MAX_RADIUS_ATTRIBUTES 64

struct RADIUS_Attribute {
   // For Vendor specific attribute (0x1A),
   // first 4 bytes of value is the vendor ID

    unsigned char        _type;
    unsigned char      _length;
    unsigned char      *_value;

    RADIUS_Attribute() {
      _type = 0, _value = 0, _length = 0;
    }

    ~RADIUS_Attribute() {
      if (_value) {
        delete[] _value;
      }
    }

    uint32_t type() const { return _type; }
    int length() const { return (int) _length; }

    // Getters
    const char *value() const { return (const char *) _value; }
    int int_value() const { return *(int *) _value; }

    // Setters
    void int_attr(unsigned char type, int value) {
      _type = type;
      _length = 4;
      _value = new unsigned char[4];
      *(int *) _value = value;
    }

    void str_attr(unsigned char type, const char *value, size_t length) {
      _type = type;
      _length = length;
      _value = new unsigned char[length];
      memcpy(_value, value, length);
    }

    // Logging support
    const char *attr_name(ushort type);
    void dump();
};

class RADIUS_Packet {
    u_char _code;
    u_char _ident;
    ushort _length;
    u_char _auth[16];

    // Attributes
    RADIUS_Attribute _attributes[MAX_RADIUS_ATTRIBUTES];

    // New packet have to be created by factory methods only
    RADIUS_Packet() {
      memset(this, 0, sizeof(*this)); //XXX
    }

public:

    static RADIUS_Packet *create_acct_response(RADIUS_Packet *request);
    static RADIUS_Packet *create_acct_request(int status_type, const char *auth_info, const char *ipaddr);

    // Get packet from buffer
    void marshall(const u_char *buffer, int len, const char *auth_salt);

    // Assemble packet back to buffer, return actual packet lengh
    void unmarshall(u_char *buffer, const char *auth_salt, int *len);

    // create packet from network received buffer
    static RADIUS_Packet *parse_request(const u_char *buffer, int len, const char *auth_salt);

    uint32_t code() const { return (uint32_t) _code; }

    const RADIUS_Attribute *attr(uint32_t p_type) {
      for (int i = 0; _attributes[i].type() != 0; ++i) {
        if (_attributes[i].type() == p_type) {
          return &(_attributes[i]);
        }
      }
      return NULL;
    }

    const char *code_name(ushort code);
    void dump();
};

#endif
