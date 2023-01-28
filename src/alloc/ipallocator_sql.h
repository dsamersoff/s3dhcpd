#ifndef _IPALLOCATOR_SQL_H_
#define _IPALLOCATOR_SQL_H_

#include <sqlite3.h>
#include <unistd.h>

#include "ipallocator.h"

class IPAllocatorSQL :
     public IPAllocator  {

  sqlite3 *_leases_db;
  sqlite3 *_static_db;

  const char *_static_get_query;

  // Internal functions
  void prepare_databases();
  int do_update_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info);
  int do_create_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info);

  // API inherited from IPAllocator
  virtual bool get_static_ip(const MACAddress& mac, IPAddress& ip, int *inet_access);
  virtual bool get_leased_ip(const MACAddress& mac, IPAddress& ip);
  virtual bool get_new_ip(const MACAddress& mac, IPAddress& ip);
  virtual void record_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info);
  virtual void record_static_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info);

public:
  IPAllocatorSQL();
  virtual ~IPAllocatorSQL();
};

#endif
