#ifndef _TOOLS_H_
#define _TOOLS_H_

/* IP primitives, yes - I know about inet_addr etc
 * TODO: Support big endian machines
 */

inline uint32_t ip_s2u(const char *s_addr) {
  char t[4] = {0,0,0,0};
  const char *s = s_addr;
  int i = 0;
  while(true) {
    if (*s == '.') { ++i; ++s; continue; }
    if (!*s) { break; }
    t[i] = (t[i]*10) + (*s-'0');
    ++s;
  }

  return *(uint32_t *)t;
}

inline char* ip_u2s(uint32_t addr, char *s_addr) {
  const unsigned char *s = (const unsigned char *)&addr;
  char *t = s_addr;

  int i = 0, j = 0;
  while(true) {
    unsigned char s1 = s[j]/100, s2 = s[j] % 100/10;
    if (s1 > 0) { t[i++] = s1 + '0'; }
    if (s1 > 0 || s2 > 0) { t[i++] = s2 + '0'; }
    t[i++] = s[j] % 10 +'0';

    if (j == 3) { break; }
    t[i++] = '.';
    j++;
  }
  t[i] = 0;
  return s_addr;
}

#define BYTE(n,m) (((uint8_t *)&(n))[m])
#define UINT32(n) (*(uint32_t*)(n))

inline  bool ip_in_subnet(uint32_t subnet, uint32_t mask, uint32_t ipaddr) {
// swap bytes
  uint8_t su[4]={ BYTE(subnet,3), BYTE(subnet,2), BYTE(subnet,1), BYTE(subnet,0) };
  uint8_t ms[4]={ BYTE(mask,3), BYTE(mask,2), BYTE(mask,1), BYTE(mask,0) };
  uint8_t ip[4]={ BYTE(ipaddr,3), BYTE(ipaddr,2), BYTE(ipaddr,1), BYTE(ipaddr,0) };

  return ((UINT32(ip) & UINT32(ms)) == UINT32(su));
}

// TODO: do it better
inline uint32_t get_ip_broadcast(uint32_t subnet, uint32_t mask) {
// swap bytes
  uint8_t su[4]={ BYTE(subnet,3), BYTE(subnet,2), BYTE(subnet,1), BYTE(subnet,0) };
  uint8_t ms[4]={ BYTE(mask,3), BYTE(mask,2), BYTE(mask,1), BYTE(mask,0) };

  uint32_t res = UINT32(su) | ~ UINT32(ms);
  uint8_t bcast[4]={ BYTE(res,3), BYTE(res,2), BYTE(res,1), BYTE(res,0) };

  return UINT32(bcast);
}

inline  uint32_t ip_isolate_addr(uint32_t mask, uint32_t ipaddr) {
// swap bytes
  uint8_t ms[4]={ BYTE(mask,3), BYTE(mask,2), BYTE(mask,1), BYTE(mask,0) };
  uint8_t ip[4]={ BYTE(ipaddr,3), BYTE(ipaddr,2), BYTE(ipaddr,1), BYTE(ipaddr,0) };

  return (UINT32(ip) & ~ UINT32(ms));
}

// TODO: support class 'B' network - only class C net is supported
inline uint32_t ip_compose_ip(uint32_t subnet, uint32_t ipaddr) {
  uint8_t su[4]={ BYTE(subnet,3), BYTE(subnet,2), BYTE(subnet,1), BYTE(subnet,0) };
  uint8_t ip[4]={ BYTE(ipaddr,0), BYTE(ipaddr,1), BYTE(ipaddr,2), BYTE(ipaddr,3) };

  uint32_t res = UINT32(ip) |  UINT32(su);
  uint8_t newip[4]={ BYTE(res,3), BYTE(res,2), BYTE(res,1), BYTE(res,0) };
  return UINT32(newip);
}

inline  uint32_t ip_swap_int(uint32_t ipaddr) {
  // swap bytes
  uint8_t ip[4]={ BYTE(ipaddr,3), BYTE(ipaddr,2), BYTE(ipaddr,1), BYTE(ipaddr,0) };
  return (UINT32(ip));
}

// 18E728F02A55

inline const char *mac_u2s(uint64_t chaddr, char *smac) {
  uint8_t umac[8] = { BYTE(chaddr,0), BYTE(chaddr,1), BYTE(chaddr,2), BYTE(chaddr,3),
                            BYTE(chaddr,4), BYTE(chaddr,5), BYTE(chaddr,6), BYTE(chaddr,7) };

  char *t = smac;
  int j = 0, i = 0;
 
  while(true){
    uint8_t a1 = (umac[i] & 0xF0) >> 4;
    uint8_t a2 = (umac[i] & 0xF);
    t[j++] = (a1 > 9) ? (a1-10) + 'A' : a1 + '0';
    t[j++] = (a2 > 9) ? (a2-10) + 'A' : a2 + '0';
    if (i == 5) { break; }
    t[j++] = ':';
    ++i;
  }
  t[j] = 0;
  return smac;
}

inline uint64_t mac_s2u(const char *s_mac) {
  uint8_t t[8] = {0,0,0,0,0,0,0,0};
  const char *s = s_mac;

  int i = 0;
  while(true) {
    if (*s == ':') { ++i; ++s; continue; }
    if (!*s || i > 6) { break; }
    if (*s > '9') {
     t[i] = (t[i] << 4) | (*s - 'A' + 10);
    }
    else {
     t[i] = (t[i] << 4) | (*s - '0');
    }
    ++s;
  }

  return *(uint64_t *)t;
}

#undef BYTES
#undef UINT32
#endif
