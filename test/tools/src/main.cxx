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
 * @summary Testing ip primitives
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "tools.h"
#include "subnets.h"

int main(int argc, char *argv[]) {
  const char * ip_list[] = {
    "10.20.0.10",
    "10.20.11.10",
    "192.168.0.100",
    "192.168.0.101",
    "192.168.0.10"
  };

  const int ip_list_size = 5;

  const char * mac_list[] = {
    "18:E7:28:F0:2A:55",
    "22:33:44:55:66:77",
    "D0:22:BE:B3:39:C6"
  };

  const int mac_list_size = 3;

  printf("Testing IP conversion\n");

  for(int i = 0; i < ip_list_size; ++i) {
    uint32_t stock_value = inet_addr(ip_list[i]);
    uint32_t my_value = ip_s2u(ip_list[i]);
    printf("s2u Test: %s %x %x\n", ip_list[i], stock_value, my_value);
    if (stock_value != my_value) {
      printf("s2u Test FAILED: %s %x %x\n", ip_list[i], stock_value, my_value);
      exit(3);
    }
    char rev_ip[20];
    ip_u2s(my_value, rev_ip);
    if (strcmp(rev_ip, ip_list[i]) != 0) {
      printf("u2s Test FAILED: %s %x %x %s\n", ip_list[i], stock_value, my_value, rev_ip);
      exit(3);
    }
  }

  printf("Testing MAC conversion\n");

  for(int i = 0; i < mac_list_size; ++i) {
    uint64_t my_value = mac_s2u(mac_list[i]);
    char rev_mac[20];
    mac_u2s(my_value, rev_mac);
    printf("u2s Test: %s %lx %s\n", mac_list[i], my_value, rev_mac);
    if (strcmp(rev_mac, mac_list[i]) != 0) {
      printf("u2s Test FAILED: %s %lx %s\n", mac_list[i], my_value, rev_mac);
      exit(3);
    }
  }

  printf("Testing IP ranges conversion\n");

  // subnet-mysubnet0-addr = 192.168.0.0
  // subnet-mysubnet0-netmask = 255.255.255.0
  // subnet-mysubnet0-gateway = 192.168.0.10
  // subnet-mysubnet0-range-start = 192.168.0.100
  // subnet-mysubnet0-range-end = 192.168.0.200


  uint32_t start_range = ip_isolate_addr(IPAddress("255.255.255.0").u_addr, IPAddress("192.168.0.100").u_addr);
  uint32_t end_range = ip_isolate_addr(IPAddress("255.255.255.0").u_addr, IPAddress("192.168.0.200").u_addr);

  printf("Dynamic ip range %d %d\n", start_range, end_range);

  for (uint32_t new_ip = start_range; new_ip < end_range; ++new_ip) {
    IPAddress scip = ip_compose_ip(IPAddress("192.168.0.0").u_addr, new_ip);
    printf("Found %s\n", scip.s_addr);
  }


  printf("Test PASSED\n");
}
