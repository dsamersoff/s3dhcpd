/*
 *
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

#if defined(__FreeBSD__) || defined(__APPLE__)
# include <net/if_dl.h>
# define USE_GETIFADDRS
#endif

#include "dsLog.h"

#include "dhcp_constants.h"
#include "dhcp_netinet.h"
#include "dhcp_net.h"
#include "ipallocator.h"

using namespace libdms5;

/**
 * Use pcap to deal with broadcast packets.
 */

pcap_t *DHCP_Net::lstn_socket;
pcap_t *DHCP_Net::bcast_socket;
int DHCP_Net::comm_socket;

// cached local values
s3_eth  DHCP_Net::hw_header;
uint32_t  DHCP_Net::my_ip;

// Send UDP broadcast using RAW socket
// Keep all structures here to avoid platform-specific issues

pcap_t *DHCP_Net::open_listening_socket(const char *dev_name) {
  char errbuf[PCAP_ERRBUF_SIZE];      // Error string
  struct bpf_program fp;              // The compiled filter
  // pcap filter expression receive only udp packets destignated to bootps
  char filter_exp[] = "udp dst port 67"; //The filter expression

  if (dev_name == NULL) {
    dev_name = pcap_lookupdev(errbuf);
    if (dev_name == NULL) {
      throw DHCP_NetException("Can't find default device - %s", errbuf);
    }
  }

  // Open the session in promiscuous mode
  pcap_t *handle = pcap_open_live(dev_name, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    throw DHCP_NetException("Can't open device %s - %s", dev_name, errbuf);
  }

  if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) {
    throw DHCP_NetException("Couldn't compile filter exp %s - %s", filter_exp, pcap_geterr(handle));
  }

  if (pcap_setfilter(handle, &fp) == -1) {
    throw DHCP_NetException("Couldn't install filter %s: %s", filter_exp, pcap_geterr(handle));
  }

  dsLog::info("Listening on pcap://%s", dev_name);

  return handle;
}

pcap_t *DHCP_Net::open_broadcast_socket(const char *dev_name) {
  char errbuf[PCAP_ERRBUF_SIZE];      // Error string

  if (dev_name == NULL) {
    dev_name = pcap_lookupdev(errbuf);
    if (dev_name == NULL) {
      throw DHCP_NetException("Can't find default device - %s", errbuf);
    }
  }

  // Open the session in non-promiscuous mode
  pcap_t *handle = pcap_open_live(dev_name, BUFSIZ, 0, 0, errbuf);
  if (handle == NULL) {
    throw DHCP_NetException("Can't open device %s - %s", dev_name, errbuf);
  }

  dsLog::info("Broadcasting on pcap://%s", dev_name);
  return handle;
}

int DHCP_Net::open_communication_socket(const char *dev_name, const char *src_ip_addr, int bind_to_address) {
  int sock = 0;
  int on = 1;

  struct sockaddr_in saddr;

  /* setup broadcast socket */
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    throw DHCP_NetException("Can't open communication socket %m");
  }

  // setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  if (bind_to_address) {
    saddr.sin_family = PF_INET;
    // It's important for MSFT client to set
    // ordinating port to bootps (67)
    saddr.sin_port = htons(DHCP_SERVER_PORT);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY); // bind socket to any interface

    // TODO: Make it working
    //  IPAddress bind_to(src_ip_addr);
    //  saddr.sin_addr.s_addr = htonl(bind_to.u_addr); // bind socket to particular interface

    if (bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) < 0) {
      throw DHCP_NetException("Can't bind communication socket to %s (%m)", src_ip_addr);
    }
    dsLog::debug("Communication socket bound to %s", src_ip_addr);
  }

  dsLog::info("Sending on udp://%s (%s)", dev_name, src_ip_addr);
  return sock;
}

int DHCP_Net::recv_packet(pcap_t *handle, u_char *buf, int maxlen) {
  struct pcap_pkthdr pcap_header;

  while(true) {
    const u_char *packet = pcap_next(handle, &pcap_header);
    if (packet) {
      int pkt_header_size = 14 /*ethernet*/ + IP_HEADER_SIZE(packet + 14) + 8 /*udp*/;

      if (pcap_header.len < pkt_header_size) {
        dsLog::info("Invalid packet from network %d vs %d", pkt_header_size, pcap_header.len);
        continue;
      }

      int udp_packet_size = pcap_header.len - pkt_header_size;
      if (udp_packet_size > maxlen) {
        dsLog::info("Too large packet from network %d vs %d", udp_packet_size, maxlen);
        continue;
      }

      memcpy(buf, packet + pkt_header_size, udp_packet_size);
      return udp_packet_size;
    }
  } // while
}

int DHCP_Net::send_broadcast_packet(pcap_t *handle, const u_char *data, int data_len) {
  int hdr_len = sizeof(s3_eth) + sizeof(s3_ip) + sizeof(s3_udphdr);
  int pkt_len = data_len + hdr_len;

  u_char pkt[pkt_len];
  memset(pkt, 0, pkt_len);

  s3_ip *ip = (s3_ip *)(pkt + sizeof(s3_eth));
  s3_udphdr *udp = (s3_udphdr *)(pkt + sizeof(s3_ip) + sizeof(s3_eth));

  memcpy(pkt, (void *)&hw_header, sizeof(s3_eth));

  ip->ip_hl = 5;
  ip->ip_v = 4;
  ip->ip_tos = 16; // IPTOS_LOWDELAY
  // Some OS migth require ip_len in host byte order
#ifdef IP_LEN_HOST_BYTEORDER
  ip->ip_len = pkt_len - sizeof(s3_eth);
#else
  ip->ip_len = htons(pkt_len - sizeof(s3_eth));
#endif
  ip->ip_id = 0;
  ip->ip_off = htons(0x4000); //DF
  ip->ip_ttl = 16;
  ip->ip_p = IPPROTO_UDP; // 17 //*
  ip->ip_sum = 0;

  ip->ip_src.s_addr = my_ip;
  // ip->ip_src.s_addr = htonl(INADDR_ANY); //*
  ip->ip_dst.s_addr = htonl(INADDR_BROADCAST); //*

  udp->uh_sport  = htons(DHCP_SERVER_PORT);
  udp->uh_dport = htons(DHCP_CLIENT_PORT);
  // Length of UDP header and data
  udp->uh_ulen = htons(sizeof(s3_udphdr) + data_len); // *
  udp->uh_sum = 0;

  memcpy(pkt + hdr_len, data, data_len);

  // IP header checksum
  ip->ip_sum =  ip_wrapsum(ip_checksum((u_char *)ip, sizeof(s3_ip), 0));

  // UDP checksum calculation, it uses cerain fields from IP header
#ifdef FIX_UDP_CHECKSUM
  u_int32_t sum = 0;
  sum = ip_checksum((u_char *) &(ip->ip_src), 2 * sizeof(ip->ip_src),\
                                IPPROTO_UDP + (u_int32_t)ntohs(udp->uh_ulen));

  sum = ip_checksum(data, data_len, sum);
  sum = ip_checksum((u_char *) udp, sizeof(s3_udphdr), sum);

  // Can't achieve correct UDP checksum calculation, fortunatly it's optional
  // for IPv4 - so leave it to zero
  // TODO: FIX it
  udp->uh_sum = ip_wrapsum(sum);
#endif

  print_packet(pkt, pkt_len);
  if (pcap_inject(handle, pkt, pkt_len) < 0) {
    // We don't abort on packet send error as it can be an intermittent condition
    dsLog::error("Can't inject packet %s", pcap_geterr(handle));
    return -1;
  }

  return 0;
}

// BSD version
void DHCP_Net::build_ethernet_header(const char *dev_name) {
  u_char *mac = NULL;

#ifdef USE_GETIFADDRS
  // use getifaddrs call to get HW address
  struct ifaddrs *ifap, *ifaptr;

  if (getifaddrs(&ifap) < 0) {
    throw DHCP_NetException("Can't getifaddrs %m");
  }

  for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
    if (!strcmp(ifaptr->ifa_name, dev_name) && (ifaptr->ifa_addr->sa_family == AF_LINK)) {
      mac = (u_char *)LLADDR((struct sockaddr_dl *)(ifaptr->ifa_addr));
      break;
    }
  }

  if (mac == NULL) {
    throw DHCP_NetException("Can't get hw address for %s %m", dev_name);
  }

#else // use ioctl call to get HW address
  int sock = -1;
  struct ifreq ifr;

  if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
    throw DHCP_NetException("Can't open IOCTL socket %m");
  }

  memset (&ifr, 0, sizeof (ifr));
  snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", dev_name);

  if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
    close(sock);
    throw DHCP_NetException("Can't get hw address for %s %m", dev_name);
  }

  close(sock);
  mac = (u_char *)(ifr.ifr_hwaddr.sa_data);
#endif

  dsLog::debug("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X", dev_name,\
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

  memcpy(hw_header.eth_shost, mac, IFHWADDRLEN);
  memset(hw_header.eth_dhost, 0xFF, IFHWADDRLEN);
  hw_header.eth_type = htons(ETHERTYPE_IP);

#ifdef USE_GETIFADDRS
  freeifaddrs(ifap);
#endif
}

int DHCP_Net::send_packet(int socket, uint32_t to_addr, const u_char *buf, int length) {
  struct sockaddr_in baddr;

  baddr.sin_family = AF_INET;
  baddr.sin_port = htons(DHCP_CLIENT_PORT);
  baddr.sin_addr.s_addr = to_addr;
  return sendto(socket, buf, length, 0, (struct sockaddr*)&baddr, sizeof(baddr));
}

void DHCP_Net::initialize(const char *dev_name, const char *src_ip_addr, int bind_to_address) {
  lstn_socket = open_listening_socket(dev_name);
  bcast_socket = open_broadcast_socket(dev_name);
  comm_socket = open_communication_socket(dev_name, src_ip_addr, bind_to_address);

  // Support PCAP raw packet injection
  // Cache my IP
  my_ip = ip_s2u(src_ip_addr);
  // Fill in ethernet header
  build_ethernet_header(dev_name);

}

void DHCP_Net::shutdown() {
  pcap_close(lstn_socket);
  pcap_close(bcast_socket);
  close(comm_socket);
}

void DHCP_Net::main_loop() {
  u_char message_buf[DHCP_MAX_MTU];
  int message_length = 0;

  dsLogScope scope("net");

  while(true) {
    if (dsLog::current_verbosity() > 4) {
      // In packet debug mode, fill packet buffer with zeroes
      // to simplify diagnostic
      memset(message_buf, 0, DHCP_MAX_MTU);
    }

    message_length = recv_packet(lstn_socket, message_buf, DHCP_MAX_MTU);
    if (message_length < 0 || message_length < BOOTP_ABSOLUTE_MIN_LEN) {
      dsLog::error("Error receiving data or message too short %d (%d, %m)", message_length, errno);
      continue;
    }

    if (! DHCP_Packet::check_magic_cookie(message_buf, message_length)) {
      dsLog::error("Error receiving data - invalid DHCP_MAGIC_COOKIE");
      continue;
    }

    DHCP_Packet *request = DHCP_Packet::parse_request(message_buf, message_length);

    dsLogPrefix prefix(request->mac().s_addr);
    dsLog::debug("** Start session for %s %s (%d bytes)", request->mac().s_addr, request->ip().s_addr, message_length);

    int dhcp_message_type = request->get_message_type();

    // Log level 5 for component dhcp cause full dump of packets received and sent
    if (dsLog::current_verbosity() > 4) {
      dsLog::print("*** Dump of REQUEST packet %s", request->msgtype_name());
      request->dump();
      dsLog::print("*** End of Dump");
    }

    DHCP_Packet *response = NULL;
    IPAllocator *allocator = IPAllocator::get_allocator();

    switch(dhcp_message_type) {
      case DHCP_DISCOVER: {
        // Reply with OFFER or NAK
        dsLog::info("DHCP_DISCOVER ...");
        IPAddress ip_addr;
        if (allocator->ip_alloc(request->mac(), ip_addr)) {
          dsLog::info("... answering with OFFER");
          response =  DHCP_Packet::create_dhcp_offer(request, ip_addr);
        }
        else {
          dsLog::info("... answering with NAK");
          response =  DHCP_Packet::create_dhcp_nak(request, ip_addr);
        }
        break;
      }
      case DHCP_RELEASE:
        dsLog::info("DHCP_RELEASE");
        allocator->ip_free(request->mac(), request->ip());
        break;
      case DHCP_INFORM: {
        dsLog::info("DHCP_INFORM ... ");
        dsLog::info("... answering with ACK2");
        IPAddress ip = request->get_requested_address();
        response =  DHCP_Packet::create_dhcp_ack2(request, ip);
        break;
      }
      case DHCP_REQUEST: {
        dsLog::info("DHCP_REQUEST ...");
        IPAddress ip = request->get_requested_address();
        dsLog::info("... requested %s", ip.s_addr);
        if (allocator->ip_check(request->mac(), ip)) {
          dsLog::info("... answering with ACK");
          response =  DHCP_Packet::create_dhcp_ack(request, ip);
        }
        else {
          dsLog::error("... answering with NAK");
          response =  DHCP_Packet::create_dhcp_nak(request, ip);
        }
        break;
      }
      case DHCP_DECLINE: {
        dsLog::info("DHCP_DECLINE ... invalidating");
        allocator->ip_invalidate(request->mac(), request->ip());
        break;
      }
      default:
        dsLog::error("Unexpected request packet type %d", dhcp_message_type);
        break;
    }

    if (response != NULL) {
      // No response for non implemented or unexpected packets
      if (dsLog::current_verbosity() > 4) {
        dsLog::print("*** Dump of RESPONSE packet %s", response->msgtype_name());
        response->dump();
        dsLog::print("*** End of Dump");
      }

      response->unmarshall(message_buf, &message_length);

      if (response->get_ciaddr() == 0){
        dsLog::debug("Broadcasting %d bytes", message_length);
        if (send_broadcast_packet(bcast_socket, message_buf, message_length) < 0) {
          dsLog::error("Error broadcasting %d bytes (%d, %m)",message_length, errno);
        }
      }
      else{
        // We come here for DHCPINFORM message only
        // RFC 2131 requires DHCPINFORM to be send to request->ciaddr but we are simplify life a bif
        // using request ip addr that should be the same
        dsLog::debug("Sending %d bytes to %s:%d",message_length, request->ip().s_addr, DHCP_CLIENT_PORT);
        if (send_packet(comm_socket, request->ip().u_addr, message_buf, message_length) < 0) {
          dsLog::error("Error sending %d bytes to %s:%d (%d, %m)",message_length, request->ip().s_addr, DHCP_CLIENT_PORT, errno);
        }
      }
      delete response;
    }

    dsLog::debug("** End session for %s %s", request->mac().s_addr, request->ip().s_addr);
    delete request;
  } // while
}
