/*
 * S3 DHCP Daemon
 *
 *
 *
 */

#include <stdio.h>
#include <unistd.h>

#include "dsSmartException.h"
#include "dsLog.h"
#include "dsGetopt.h"
#include "dsSlightFunctions.h"

#include "daemon.h"
#include "dhcp_net.h"
#include "subnets.h"
#include "ipallocator.h"

#ifdef WITH_RADIUS_ACCT
# include "radius_net.h"
#endif

#ifdef WITH_PF
# include "pf.h"
#endif

#define PROGNAME "s3dhcpd"

extern const char *VERSION();

using namespace libdms5;

void usage(const char *progname) {
  printf("Usage: %s -c config_file_name\n", progname);
  exit(7);
}

int main(int argc, char*argv[]) {

  printf("%s\n", VERSION());
  int c;
  const char * _config_file = NULL;
  const char * _interface = NULL;
  bool _daemonise = false;

  try {
    dsGetopt gto(argc, argv);

    while ((c = gto.next("hc:di:k")) != EOF){
      switch (c){
        case 'c':
          _config_file = dsStrdup(gto.optarg()); break;
        case 'i':
          _interface = dsStrdup(gto.optarg()); break;
        case 'd':
          _daemonise = true; break;
        case 'k':
          // TODO: Daemon::kill_running(PROGNAME); exit(0);
        case 'h':
        case '?':
        default:
          usage(argv[0]);
      }
    }
  }
  catch(dsGetoptException& e) {
    e.print();
    usage(argv[0]);
  }

  if (_config_file == NULL) {
    dsLog::error("No config file specified. Exiting");
    exit(7);
  }

  if (_interface == NULL) {
    dsLog::error("No interface specified. Exiting");
    exit(7);
  }

  // Initialize timezone for logging
  tzset();

  try {
    // Load rc file
    dsApprc *rc = dsApprc::global_rc();
    rc->load(_config_file);

    const char *logname = rc->getstring("log-filename", "cerr");
    // TODO: Do it better
    char progname[sizeof(PROGNAME) + strlen(_interface) + 16];
    sprintf(progname,"%s_%s", PROGNAME, _interface);

    // Daemonize close all file descriptors, so we have to go to background right
    // after loading rc file.
    // TODO: be able to set pid file path
    dsLog::print("%s\n", VERSION());
    dsLog::print("As %s. See %s", progname, logname);
    if (_daemonise) {
      Daemon::daemonize(progname,0);
    }

    dsLog::set_output(logname);
    dsLog::push_component("init");

    // Load subnet configuration
    Subnets::load();

    // Initialize networking
    const char *server_addr = rc->getstring("global-server-addr");
    int bind_to_address = rc->getbool("global-bind-to-address", false);

    DHCP_Net::initialize(_interface,server_addr, bind_to_address);

#ifdef WITH_RADIUS_ACCT
    int radius_enabled = rc->getbool("radius-enabled", false);
    if (radius_enabled) {
      const char *radius_server = rc->getstring("radius-server-addr");
      const char *radius_auth_salt = rc->getstring("radius-auth-salt");
      int radius_port = rc->getint("radius-server-port", RADIUS_ACCT_PORT);
      RADIUS_Net::initialize(radius_server, radius_port, radius_auth_salt);
    }
#endif

#ifdef WITH_PF
    int pf_enabled = rc->getbool("pf-enabled", false);
    if (pf_enabled) {
      const char *pf_dev = rc->getstring("pf-device");
      const char *pf_table = rc->getstring("pf-default-table");
      PFTable::initialize(pf_dev, pf_table);
    }
#endif

    //TODO: change umask and down to unprivileged user before
    // any database work

    // Create and initialize ip allocator
    IPAllocator::initialize();

    dsLog::print("*** Server initilization done.");
    dsLog::pop_component();
    
    DHCP_Net::main_loop();
  }
  catch(dsSmartException &e) {
    dsLog::print("FATAL %s", e.msg());
    dsLog::print("** Shutting down !!!");
    if (! _daemonise) {
      // Print fatal errors to stderr if we are not a daemon
      e.print();
    }

    return 1;
  }

  return 0;
}
