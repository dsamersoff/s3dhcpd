#ifndef _SUBNETS_H_
#define _SUBNETS_H_

#include <string.h>
#include <netinet/in.h>

#include "dsSmartException.h"

#include "tools.h"

DECLARE_EXCEPTION(Subnet);

class Subnet;

struct MACAddress {
  // u_mac is 6 byte, s_mac is 18 symbols
  // 18:e7:28:f0:2a:55
  uint64_t u_addr;
  char s_addr[20];

  MACAddress(const MACAddress& mac) {
    strcpy(s_addr, mac.s_addr);
    u_addr = mac.u_addr;
  }

  MACAddress(uint64_t p_uaddr) {
    u_addr = p_uaddr;
    mac_u2s(p_uaddr,s_addr);
  }

  MACAddress(const char * p_saddr) {
    strcpy(s_addr, p_saddr);
    u_addr = mac_s2u(s_addr);
  }

};


struct IPAddress {
  char s_addr[20];
  uint32_t u_addr;

  /**
   * Object construction routines
   */

  IPAddress(const char *p_saddr) {
    strcpy(s_addr, p_saddr);
    u_addr = ip_s2u(p_saddr);
  }

  IPAddress(uint32_t p_uaddr) {
    u_addr = p_uaddr;
    ip_u2s(p_uaddr, s_addr);
  }

  IPAddress(const IPAddress& ip) {
    strcpy(s_addr, ip.s_addr);
    u_addr = ip.u_addr;
  }

  IPAddress() {
    *s_addr = 0;
    u_addr = INADDR_NONE;
  }

  bool operator!= (const IPAddress& ip) const {
    return u_addr != ip.u_addr;
  }

};

class Subnet {
  friend class Subnets;

public:
  char *name;
  const char *domain_name;

  IPAddress addr;
  IPAddress netmask;
  IPAddress gateway;
  IPAddress nameserver1;
  IPAddress nameserver2;

  IPAddress range_start;
  IPAddress range_end;

  uint32_t lease_time;
  uint32_t renew_time;

  // Broadcast address for this subnet,
  // have to be computed at loading time
  IPAddress broadcast;

  Subnet *next;

  bool myaddress(const IPAddress& ip) {
    return ip_in_subnet(addr.u_addr, netmask.u_addr, ip.u_addr);
  }

  IPAddress compose_ip(uint32_t new_ip) {
    return IPAddress(ip_compose_ip(addr.u_addr, new_ip));
  }

  uint32_t u_addr() {
    return addr.u_addr;
  }

  const char *s_addr() {
    return addr.s_addr;
  }

  bool has_dynamic_range() {
    return (range_start.u_addr != INADDR_NONE);
  }

  Subnet() {
    next = NULL;
    name = 0;
    domain_name = 0;
  }

};

class Subnets {
   static Subnet *_subnets;
   static Subnet *_global_net;

   Subnets(){ }
public:
   static void load();
   static Subnet* subnets() { return _subnets; }
   
   static Subnet *mysubnet(const IPAddress& ip);
   static Subnet *globalnet() { return _global_net; }
};



#endif
