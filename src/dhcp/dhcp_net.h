#ifndef _DHCP_NET_H_
#define _DHCP_NET_H_

#include <pcap.h>
#include <unistd.h>

#include "dhcp_packet.h"
#include "dsSmartException.h"

#include "dhcp_netinet.h"

DECLARE_EXCEPTION(DHCP_Net);

class DHCP_Net {
#ifdef UNIT_TEST
public:
#endif

  static pcap_t *lstn_socket;
  static pcap_t *bcast_socket;
  static int comm_socket;

  static s3_eth hw_header;
  static uint32_t my_ip;

  static void build_ethernet_header(const char *dev_name);
 
  static pcap_t *open_listening_socket(const char *dev_name);
  static pcap_t *open_broadcast_socket(const char *dev_name);
  static int open_communication_socket(const char *dev_name, const char* src_ip_addr, int bind_to_address);

  static int recv_packet(pcap_t *handle, u_char *buf, int maxlen);
  static int send_broadcast_packet(pcap_t *handle, const u_char *buf, int length);
  static int send_packet(int socket, uint32_t to_addr, const u_char *buf, int length);

public:
  static void initialize(const char *dev_name, const char *src_ip_addr, int bind_to_address);
  static void main_loop();
  static void shutdown();
};


#endif
