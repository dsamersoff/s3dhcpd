#ifndef _DHCP_NETINET_H_
#define _DHCP_NETINET_H_

#include <stdio.h>
#include <sys/cdefs.h>

/**
 *
 * Different platforms used to have different names for ip/udp header fields.
 * Use internal one to minimize platfrom dependency
 */

#define ETHERNET_HEADER_SIZE            14
#define IFHWADDRLEN                      6

// TODO: support big endian machines
#define IP_HEADER_SIZE(pkt) ((*(pkt) & 0xF) * 4)

#define ETHER_ADDR_LEN          6
#define ETHERTYPE_8023          0x0004  /* IEEE 802.3 packet */
#define ETHERTYPE_IP            0x0800          /* Internet Protocol packet     */

#define __PACKED __attribute__ ((packed))

struct s3_eth {
  u_char  eth_dhost[ETHER_ADDR_LEN];
  u_char  eth_shost[ETHER_ADDR_LEN];
  u_short eth_type;
} __PACKED;

/*
 * Definitions for internet protocol version 4.
 *
 * Per RFC 791, September 1981.
 */
#define IPVERSION       4

/*
 * Structure of an internet header, naked of options.
 */
struct s3_ip {
#if BYTE_ORDER == LITTLE_ENDIAN
  u_char  ip_hl:4,                /* header length */
  ip_v:4;                         /* version */
#endif
#if BYTE_ORDER == BIG_ENDIAN
  u_char  ip_v:4,                 /* version */
  ip_hl:4;                        /* header length */
#endif
  u_char  ip_tos;                 /* type of service */
  u_short ip_len;                 /* total length */
  u_short ip_id;                  /* identification */
  u_short ip_off;                 /* fragment offset field */
#define IP_RF 0x8000              /* reserved fragment flag */
#define IP_DF 0x4000              /* dont fragment flag */
#define IP_MF 0x2000              /* more fragments flag */
#define IP_OFFMASK 0x1fff         /* mask for fragmenting bits */
  u_char  ip_ttl;                 /* time to live */
  u_char  ip_p;                   /* protocol */
  u_short ip_sum;                 /* checksum */
  struct  in_addr ip_src,ip_dst;  /* source and dest address */
} __PACKED;

/*
 * UDP protocol header.
 * Per RFC 768, September, 1981.
 */
struct s3_udphdr {
  u_short uh_sport;               /* source port */
  u_short uh_dport;               /* destination port */
  u_short uh_ulen;                /* udp length */
  u_short uh_sum;                 /* udp checksum */
} __PACKED;

inline u_int32_t ip_checksum(const u_char *buf, unsigned nbytes, u_int32_t sum) {
  uint i;

  /* Checksum all the pairs of bytes first... */
  for (i = 0; i < (nbytes & ~1U); i += 2) {
    sum += (u_int16_t)ntohs(*((u_int16_t *)(buf + i)));
    if (sum > 0xFFFF) {
      sum -= 0xFFFF;
    }
  }

  /*
   * If there's a single byte left over, checksum it, too.
   * Network byte order is big-endian, so the remaining byte is
   * the high byte.
   */
  if (i < nbytes) {
    sum += buf[i] << 8;
    if (sum > 0xFFFF) {
      sum -= 0xFFFF;
    }
  }

  return (sum);
}

inline u_int32_t ip_wrapsum(u_int32_t sum) {
  sum = ~sum & 0xFFFF;
  return (htons(sum));
}

inline void print_packet(const u_char *packet, int len) {

  s3_eth *eth = (s3_eth *) packet;
  s3_ip *iph = (s3_ip *) (packet + sizeof(s3_eth));
  s3_udphdr *udph = (s3_udphdr *)(packet + sizeof(s3_ip) + sizeof(s3_eth));


  u_char *mac = eth->eth_dhost;
  printf("\nEthernet Header\n");
  printf("Dest MAC            : %02X:%02X:%02X:%02X:%02X:%02X\n", \
                                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  mac = eth->eth_shost;
  printf("Src  MAC            : %02X:%02X:%02X:%02X:%02X:%02X\n", \
                               mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  if (len <= sizeof(s3_eth)) {
     return;
  }

  printf("\nIP Header\n");
  printf("   Header len       : %d\n" , iph->ip_hl);
  printf("   Packet len       : %d\n" , iph->ip_len);
  printf("   Protocol         : %d\n" , iph->ip_p & 0xFF);
  printf("   TTL              : %d\n" , iph->ip_ttl & 0xFF);
  printf("   Checksum         : 0x%X\n", ntohs(iph->ip_sum));
  printf("   Source IP        : 0x%X\n" , ntohl(iph->ip_src.s_addr));
  printf("   Dest IP          : 0x%X\n" , ntohl(iph->ip_dst.s_addr));

  if (len <= sizeof(s3_eth) + sizeof(s3_ip)) {
     return;
  }

  printf("\nUDP Header\n");
  printf("   Source Port      : %d\n" , ntohs(udph->uh_sport));
  printf("   Destination Port : %d\n" , ntohs(udph->uh_dport));
  printf("   UDP Length       : %d\n" , ntohs(udph->uh_ulen));
  printf("   UDP Checksum     : 0x%X\n" , ntohs(udph->uh_sum));
  printf("\n");

  //Move the pointer ahead and reduce the size of string
  //print_data(packet + header_size , size - header_size);
}


#endif
