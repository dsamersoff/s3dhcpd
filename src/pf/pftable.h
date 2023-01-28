#ifndef _PFTABLE_H_
#define _PFTABLE_H_

/* OpenBSD PF table manipulation routines.
 * With many thanks to Armin Wolfermann
 *
 * To add single ip set mask to 32
 */

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/pfvar.h>

#include <string.h>

inline int pftable_add(int pfd, const char *tname, uint32_t ip, uint8_t mask) {
  struct pfioc_table io;
  struct pfr_table table;
  struct pfr_addr addr;
  struct pftimeout *t;

  memset(&io, 0, sizeof(io));
  memset(&table,0, sizeof(table));
  memset(&addr, 0, sizeof(addr));

  // If table name is too long and lost last 0, pf internal
  // validation routine handle it and return EINVAL
  strncpy(table.pfrt_name, tname, sizeof(table.pfrt_name));
  memcpy(&addr.pfra_ip4addr, &ip, 4);

  addr.pfra_af = AF_INET;
  addr.pfra_net = mask;

  io.pfrio_table = table;
  io.pfrio_buffer = &addr;
  io.pfrio_esize = sizeof(addr);
  io.pfrio_size = 1;

  return ioctl(pfd, DIOCRADDADDRS, &io);
}

inline int pftable_del(int pfd, const char *tname, uint32_t ip, uint8_t mask) {
  struct pfioc_table io;
  struct pfr_table table;
  struct pfr_addr addr;

  memset(&io, 0, sizeof(io));
  memset(&table,0, sizeof(table));
  memset(&addr, 0, sizeof(addr));

  // If table name is too long and lost last 0, pf internal
  // validation routine handle it and return EINVAL
  strncpy(table.pfrt_name, tname, sizeof(table.pfrt_name));
  memcpy(&addr.pfra_ip4addr, &ip, 4);

  addr.pfra_af = AF_INET;
  addr.pfra_net = mask;

  io.pfrio_table = table;
  io.pfrio_buffer = &addr;
  io.pfrio_esize = sizeof(addr);
  io.pfrio_size = 1;

  return ioctl(pfd, DIOCRDELADDRS, &io);
}

inline int pftable_flush(int pfd, const char *tname) {
  struct pfioc_table io;
  struct pfr_table table;

  memset(&io, 0, sizeof(io));
  memset(&table,0, sizeof(table));

  // If table name is too long and lost last 0, pf internal
  // validation routine handle it and return EINVAL
  strncpy(table.pfrt_name, tname, sizeof(table.pfrt_name));

  io.pfrio_table = table;

  return ioctl(pfd, DIOCRCLRADDRS, &io);
}

#endif
