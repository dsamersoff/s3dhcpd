#ifndef _IP_ALLOCATOR_H_
#define _IP_ALLOCATOR_H_

#include "dsSmartException.h"
#include "subnets.h"

DECLARE_EXCEPTION(IPAllocator);

class IPAllocator {

  static IPAllocator *current_allocator;

protected:
  // Use factory method to get an allocator
  IPAllocator() {
    // pass
  }

  virtual bool get_static_ip(const MACAddress& mac, IPAddress& ip, int *access);
  virtual bool get_leased_ip(const MACAddress& mac, IPAddress& ip);
  virtual bool get_new_ip(const MACAddress& mac, IPAddress& ip);

  // Record new lease
  // TODO: It might be a necessary performance optimization to don't record
  // offered credentials to database but keep it in memory until DHCPREQUEST
  // message comes.

  virtual void record_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info);
  virtual void record_static_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info);

  // Update existing lease with new expiration time, don't change auth info
  virtual void update_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
    record_lease(mac, ip, auth_info);
  }

  virtual void update_static_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
    record_static_lease(mac, ip, auth_info);
  }

public:

  // Reserve IP address
  virtual bool ip_alloc(const MACAddress& mac, IPAddress& ip);

  // Check that ip address is allocated
  virtual bool ip_check(const MACAddress& mac, const IPAddress& ip);

  // Return IP address to pool immediately
  virtual void ip_free(const MACAddress& mac, const IPAddress& ip);

  // Mark this IP address as not available for next lease cycle
  virtual void ip_invalidate(const MACAddress& mac, const IPAddress& ip);

  // Prepare database, open file descriptors etc
  static void initialize();
  static void shutdown();

  // Factory function
  static IPAllocator * get_allocator();
};

#endif
