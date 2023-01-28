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
 * @summary Testing ip allocation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "dsSmartException.h"
#include "dsApprc.h"
#include "dsLog.h"

#include "ipallocator.h"
#include "subnets.h"

#include "tools.h"

using namespace libdms5;

int main(int argc, char *argv[]) {
  const char * mac_list1[] = {
    "18:E7:28:F0:2A:55",
    "22:33:44:55:66:77",
    "08:11:96:BE:8B:C4"
  };

  const int mac_list1_size = 3;

  const char * mac_list2[] = {
    "18:E7:28:F0:2A:55",
    "22:33:44:55:66:77",
    "88:99:44:55:66:22"
  };

  const int mac_list2_size = 3;

  const char * ip_list2[] = {
    "10.20.0.77",
    "10.20.11.50",
    "10.20.11.51"
  };


  // Initialization
  try {
    // Load rc file
    dsApprc *rc = dsApprc::global_rc();
    rc->load("test.rc");
    dsLog::set_output(rc->getstring("log-filename", "cerr"));
    dsLog::push_component("init");

    // Load subnet configuration
    Subnets::load();

    // Create and initialize ip allocator
    IPAllocator::initialize();
    dsLog::pop_component();
  }
  catch(dsSmartException &e) {
    e.print();
    return 7;
  }

  IPAllocator *alloc = IPAllocator::get_allocator();

  dsLog::print("Initialization done");
  try {
    for(int i = 0; i < mac_list1_size; ++i) {
      MACAddress mac(mac_list1[i]);
      IPAddress ip;
      if (alloc->ip_alloc(mac, ip)) {
        printf("Allocated %s %s\n", mac.s_addr, ip.s_addr);
      }
      else {
        printf("NOT Allocated %s\n", mac.s_addr);
      }
    }

    printf("Waiting for expire ... 15 sec\n");
    safe_sleep(15);

    for(int i = 0; i < mac_list2_size; ++i) {
      MACAddress mac(mac_list2[i]);
      IPAddress ip;
      if (alloc->ip_alloc(mac, ip)) {
        printf("**** Allocated %s %s\n", mac.s_addr, ip.s_addr);
        if (ip != IPAddress(ip_list2[i])) {
          printf("Test FAILED: ip is %s but should be %s\n", ip.s_addr, ip_list2[i]);
          exit(3);
        }
      }
      else {
        printf("**** NOT Allocated %s\n", mac.s_addr);
      }
    }

   }
  catch(dsSmartException &e) {
    e.print();
    return 1;
  }


  printf("Test PASSED\n");
}
