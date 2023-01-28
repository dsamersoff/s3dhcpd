/**
 *
 *
 *
 *
 *
 *
 */

/*
 * @test
 * @summary Testing BPF code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <netinet/ip.h>
#include <netinet/udp.h>

#include <pcap.h>

#include "dhcp_net.h"
#include "dhcp_netinet.h"
#include "dhcp_constants.h"

#include "test_packets.h"

using namespace libdms5;

int mypcap_work(int argc, char *argv[]) {
  pcap_t *handle;                 /* Session handle */
  char *dev;                      /* The device to sniff on */
  char errbuf[PCAP_ERRBUF_SIZE];  /* Error string */
  struct bpf_program fp;          /* The compiled filter */
  char filter_exp[] = "port 67";  /* The filter expression */
  bpf_u_int32 mask;               /* Our netmask */
  bpf_u_int32 net;                /* Our IP */
  struct pcap_pkthdr header;      /* The header that pcap gives us */
  const u_char *packet;           /* The actual packet */

  /* Define the device */
  if (argc > 1) {
    dev = argv[1];
  }
  else {
    dev = pcap_lookupdev(errbuf);
    if (dev == NULL) {
      fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
      return(2);
    }
  }
    /* Find the properties for the device */
  if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
    fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
    net = 0;
    mask = 0;
  }
  /* Open the session in promiscuous mode */
  handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
    return(2);
  }
  /* Compile and apply the filter */
  if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
    fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
    return(2);
  }
  if (pcap_setfilter(handle, &fp) == -1) {
    fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
    return(2);
  }

  while(true) {
    /* Grab a packet */
    packet = pcap_next(handle, &header);
    /* Print its length */
    printf("Jacked a packet with length of [%d]\n", header.len);
    if (packet) {
      print_packet(packet + ETHERNET_HEADER_SIZE, 1024);
    }
  }

  /* And close the session */
  pcap_close(handle);
  return(0);
}

void
assemble_udp_ip_header(unsigned char *buf, int *bufix, u_int32_t from,
                       u_int32_t to, unsigned char *data, int len);


int main(int argc, char *argv[]) {

  if (argc != 2) {
    printf("Usage: test ifname\n");
    exit(7);
  }

  try {
    DHCP_Net::initialize(argv[1], "192.168.0.77" /* argv[2] */);

#ifdef WITH_BSD_PACKET_ASSEMBLER
    u_char buf[4096];
    int bufix = 0;

    assemble_udp_ip_header(buf, &bufix, inet_addr("192.168.0.77"), htonl(INADDR_BROADCAST), \
                           (u_char *)discovery_packet, discovery_packet_length);

    printf("BSD packet .....\n");
    print_packet(buf, bufix);

    memcpy(buf + bufix, discovery_packet, discovery_packet_length);
    send(DHCP_Net::comm_socket, buf, bufix + discovery_packet_length, 0);
#endif


#ifdef SEND_BCAST_PACKET
    print_packet(bcast_packet, bcast_packet_length);
    int rc;
    if ((rc = pcap_inject(DHCP_Net::comm_socket, bcast_packet, bcast_packet_length)) < 0) {
      pcap_perror(DHCP_Net::comm_socket, (char *) "Failed to inject packet");
      return -1;
    }
#endif

    int n_packets = 1;
    for (int i = 0; i < n_packets; ++i) {
      printf("S3 packet .....\n");
      // Send discovery packet
      DHCP_Net::send_packet(DHCP_Net::comm_socket, 0, discovery_packet, discovery_packet_length);
    }

#if 0
    char message_buf[DHCP_MAX_MTU];
    int message_length = 0;

    while(true) {
      message_length = DHCP_Net::recv_packet(DHCP_Net::dhcp_handle, message_buf, DHCP_MAX_MTU);

      DHCP_Packet *request = DHCP_Packet::parse_request(message_buf, message_length);
      request->dump();
    }
#endif
  }
  catch(dsSmartException& e) {
    e.print();
    exit(3);
  }

  printf("Test PASSED\n");
  return 0;
}
