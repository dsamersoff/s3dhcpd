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
 * @summary Testing packet engine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "dsLog.h"

#include "dhcp_packet.h"
#include "dhcp_constants.h"

#include "test_packets.h"

using namespace libdms5;

int main(int argc, char *argv[]) {

  // Initialization
  try {
    // Load rc file
    dsApprc *rc = dsApprc::global_rc();
    rc->load("test.rc");
    dsLog::set_output(rc->getstring("log-filename", "cerr"));
    dsLog::push_component("init");

    // Load subnet configuration
    Subnets::load();

    dsLog::pop_component();
  }
  catch(dsSmartException &e) {
    e.print();
    return 7;
  }

  try {
    DHCP_Packet *request = DHCP_Packet::parse_request(discovery_packet, discovery_packet_length);
    request->dump();

    dsLog::push_component("packet");

    int dhcp_message_type = request->get_message_type();

    if (dhcp_message_type != DHCP_DISCOVER) {
      printf("Test FAILED: Incorrect message type %d vs %d\n", dhcp_message_type, DHCP_DISCOVER);
      exit(3);
    }

    char message_buf[DHCP_MAX_MTU];
    int message_length = 0;

    request->unmarshall(message_buf, &message_length);

    // Unmarshalled packet is always padded up to 300 bytes, but original packet may don't have a padding
    // So, length check is meaningless

    // if (message_length != discovery_packet_length) {
    //  printf("Test FAILED: Incorrect paccket length %d vs %d\n", message_length, discovery_packet_length);
    //  exit(3);
    // }

    if (memcmp(message_buf, discovery_packet, discovery_packet_length) != 0) {
      printf("Test FAILED: Incorrect paccket\n");
      exit(3);
    }

    DHCP_Packet *response;

    response = DHCP_Packet::create_dhcp_offer(request, IPAddress("10.20.0.77"));
    response->dump();
    delete response;

    response = DHCP_Packet::create_dhcp_ack(request, IPAddress("10.20.0.77"));
    response->dump();

    delete response;

    response = DHCP_Packet::create_dhcp_nak(request, IPAddress("10.20.0.77"));
    response->dump();
    delete response;

    delete request;

    dsLog::pop_component();
  }
  catch(dsSmartException &e) {
    e.print();
    exit(3);
  }

  printf("Test PASSED\n");
}
