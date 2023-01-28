#ifndef _RADIUS_NET_H_
#define _RADIUS_NET_H_

#include <netinet/in.h>

#include "subnets.h"

#include "radius_packet.h"
#include "dsSmartException.h"

DECLARE_EXCEPTION(Net);

class RADIUS_Net {
#ifdef UNIT_TEST
public:
#endif

  static bool enabled;
  static int comm_socket;
  static char *auth_salt;
  static IPAddress *server_addr;
  static int server_port;

  static int open_communication_socket();
  static int send_packet(int socket, uint32_t to_addr, int port, const u_char *buf, int length);

public:
  static void initialize(const char *server_addr, int server_port, const char *auth_salt);
  static void shutdown();
  static void send_acct_message(int status_type, const char *auth_info, const char *ipaddr);

  static bool rad_enabled(){ return enabled; }

};


#endif
