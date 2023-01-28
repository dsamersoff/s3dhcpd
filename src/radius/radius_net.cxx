/*
 *
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>


#include "dsLog.h"

#include "radius_constants.h"
#include "radius_net.h"

using namespace libdms5;

/**
 * Use plain socket API
 */

bool RADIUS_Net::enabled = false;
int RADIUS_Net::comm_socket;
char *RADIUS_Net::auth_salt = NULL;

IPAddress *RADIUS_Net::server_addr = NULL;
int RADIUS_Net::server_port = -1;

// We are sending radius packets blindly, without retring.
int RADIUS_Net::open_communication_socket() {
  int the_socket = 0;
  /* setup broadcast socket */
  if((the_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    throw NetException("Can't open communication socket %m");
  }
  return the_socket;
}

int RADIUS_Net::send_packet(int socket, uint32_t to_addr, int port, const u_char *buf, int length) {
  struct sockaddr_in baddr;

  baddr.sin_family = AF_INET;
  baddr.sin_port = htons(port);
  baddr.sin_addr.s_addr = to_addr;
  return sendto(socket, buf, length, 0, (struct sockaddr*)&baddr, sizeof(baddr));
}

void RADIUS_Net::initialize(const char *p_server_addr, int p_port, const char *p_auth_salt) {
  dsLogScope("radius");

  comm_socket = open_communication_socket();
  auth_salt = dsStrdup(p_auth_salt);

  // TODO: Perform NS lookup here
  server_addr = new IPAddress(p_server_addr);
  server_port = p_port;

  dsLog::info("Radius accounting enabled. Sending to %s:%d", p_server_addr, p_port);
  enabled = true;
}

void RADIUS_Net::shutdown() {
  close(comm_socket);

  if (auth_salt) {
    delete[] auth_salt;
  }

  if (server_addr) {
    delete server_addr;
  }
}

void RADIUS_Net::send_acct_message(int status_type, const char *auth_info, const char *ipaddr) {
  dsLogScope("radius");

  RADIUS_Packet *request = RADIUS_Packet::create_acct_request(status_type, auth_info, ipaddr);
  if (request == NULL) {
    dsLog::error("Error building radius acct message");
    return;
  }

  u_char message_buf[1024];
  int message_length = 0;

  // No response for non implemented or unexpected packets
  if (dsLog::current_verbosity() > 4) {
    dsLog::print("*** Dump of RADIUS packet");
    request->dump();
    dsLog::print("*** End of Dump");
  }

  request->unmarshall(message_buf, auth_salt, &message_length);

  dsLog::debug("Sending radius acct message %d bytes to %s:%d",message_length, server_addr->s_addr, server_port);

  if (send_packet(comm_socket, server_addr->u_addr, server_port, message_buf, message_length) < 0) {
    dsLog::error("Error sending %d bytes to %s:%d (%d, %m)",message_length, server_addr->s_addr, server_port, errno);
  }

  delete request;
}
