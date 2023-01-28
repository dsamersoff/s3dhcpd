/*
 *
 *
 *
 */

#include "dsLog.h"

#include "ipallocator.h"
#include "ipallocator_sql.h"
#include "tools.h"

#ifdef WITH_RADIUS_ACCT
# include "radius_net.h"
#endif

#ifdef WITH_PF
# include "pf.h"
#endif

using namespace libdms5;

IPAllocator *IPAllocator::current_allocator = 0;

/**
 * DHCP server has the only active allocator for now.
 * and it's the only place you need to change to
 * add your own allocator
 */

IPAllocator * IPAllocator::get_allocator() {
  // Guard agains non-correct usage,
  // better to run initialize first
  if (current_allocator == 0) {
    current_allocator = new IPAllocatorSQL();
  }
  return current_allocator;
}

void IPAllocator::initialize() {
  if (current_allocator == 0) {
    current_allocator = new IPAllocatorSQL();
  }
}

void IPAllocator::shutdown() {
  if (current_allocator) {
    delete current_allocator;
  }
}


bool IPAllocator::get_static_ip(const MACAddress& mac, IPAddress& ip, int *inet_access) {
  char new_name[64];
  strcpy(new_name,"host-");
  strcat(new_name, mac.s_addr);

  const char *str_ip = dsApprc::global_rc()->getstring(new_name, (const char *)NULL);
  if (str_ip != NULL) {
    ip = IPAddress(str_ip);

    // Check whether the static entry has access to internet, NO by default
    strcat(new_name,"-inet");
    *inet_access = (dsApprc::global_rc()->getbool(new_name, 0));
    return true;
  }

  return false;
}

bool IPAllocator::get_leased_ip(const MACAddress& mac, IPAddress& ip) {
  return false;
}

bool IPAllocator::get_new_ip(const MACAddress& mac, IPAddress& ip) {
  return false;
}

void IPAllocator::record_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
  dsLog::debug("Default record_lease called for %s", ip.s_addr);
}

void IPAllocator::record_static_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
  dsLog::debug("Default record_static_lease called for %s", ip.s_addr);
}

bool IPAllocator::ip_alloc(const MACAddress& mac, IPAddress& ip){
  dsLogScope scope("alloc");
  dsLog::debug("Request ip allocation for '%s'", mac.s_addr);

  // 1. Check static database
  int inet_access = 0;
  if (get_static_ip(mac, ip, &inet_access)) {
    // Static leases is not recorded anyway.
    // TODO: Record static leases to leases database
    if (inet_access) {
      dsLog::debug("Allocated from STATIC: '%s' inet: Yes", ip.s_addr);
      record_static_lease(mac, ip, "static");
    }
    else {
      dsLog::debug("Allocated from STATIC: '%s' inet: No", ip.s_addr);
      record_static_lease(mac, ip, "internal");
    }

#ifdef WITH_PF
    if (inet_access) {
      // This static entry has access to internet
      if (PFTable::pf_enabled()) {
        dsLog::debug("Adding static %s to pf admitted list", ip.s_addr);
        PFTable::add(ip.u_addr);
      }
    }
#endif

#ifdef WITH_RADIUS_ACCT
    // IP address allocated from static. Send START_SESSION_MESSAGE
    if (RADIUS_Net::rad_enabled()) {
      dsLog::debug("Sending RAD_SESSION_DHCP_STAR for static %s (%s)", ip.s_addr, mac.s_addr);
      RADIUS_Net::send_acct_message(RAD_SESSION_DHCP_START,  mac.s_addr, ip.s_addr);
    }
#endif

    return true;
  }

  // 2. Check leased database
  if (get_leased_ip(mac, ip)) {
    // Update lease with new time
    update_lease(mac, ip, "leased");
    dsLog::debug("Allocated from LEASED: '%s'", ip.s_addr);
    return true;
  }

  // 3. Allocate new IP
  if (get_new_ip(mac, ip)) {
    // Update lease with new mac and time
    // Reset any auth information associated to allocation
    record_lease(mac, ip, "new");
    dsLog::debug("Allocated from NEW: '%s'", ip.s_addr);

  // Remove allocated IP address from admitted pf table
#ifdef WITH_PF
    if (PFTable::pf_enabled()) {
      dsLog::debug("Removing %s from pf admitted list", ip.s_addr);
      PFTable::del(ip.u_addr);
    }
#endif

#ifdef WITH_RADIUS_ACCT
    // IP address leased to another client. Send START_SESSION_MESSAGE
    if (RADIUS_Net::rad_enabled()) {
      dsLog::debug("Sending RAD_SESSION_DHCP_STAR for %s (%s)", ip.s_addr, mac.s_addr);
      RADIUS_Net::send_acct_message(RAD_SESSION_DHCP_START, mac.s_addr, ip.s_addr);
    }
#endif

    return true;
  }

  // No IP
  dsLog::debug("No allocation possible");
  return false;
}

bool IPAllocator::ip_check(const MACAddress& mac, const IPAddress& ip){
  dsLogScope scope("alloc");
  dsLog::debug("Check ip allocation for '%s' '%s'", mac.s_addr, ip.s_addr);

  // 0. Defend from foreign leases
  Subnet *sub = Subnets::mysubnet(ip);
  if (sub == NULL) {
    // requested IP comes from foreighn subnet.
    // it could be result of server configuration change
    dsLog::info("Declined foreign IP '%s'", ip.s_addr);
    return false;
  }

  IPAddress newip;
  int inet_access = 0;
  // 1. Check static database
  if (get_static_ip(mac, newip, &inet_access) ) {
    dsLog::debug("Found in STATIC");
    if (ip != newip) {
      dsLog::info("Declined invalid IP '%s' (should be '%s')", ip.s_addr, newip.s_addr);
      return false;
    }

    // Static leases doesn't require any auth. So always set it to word static
    update_static_lease(mac, ip, "static");
    return true;
  }

  if (!sub->has_dynamic_range()) {
    // request IP come from subnet that doesn't have dynamic range configured,
    // but client mac address is not known. It could be result of database change.
    // so don't query leases databes
    dsLog::info("Declined unknown mac in static subnet for '%s' '%s'", mac.s_addr, ip.s_addr);
    return false;
  }

  // 2. Check leased database
  if (get_leased_ip(mac, newip) ) {
    dsLog::debug("Found in LEASED");
    if (ip != newip) {
      dsLog::info("Declined invalid IP '%s' (should be '%s')", newip.s_addr, ip.s_addr);
      return false;
    }

    // Update lease time and expiration
    update_lease(mac, ip, NULL);
    return true;
  }

  dsLog::debug("Not found");
  return false;
}

void IPAllocator::ip_free(const MACAddress& mac, const IPAddress& ip){
#ifdef WITH_RADIUS_ACCT
    // Client release IP address. Send END_SESSION_MESSAGE
    if (RADIUS_Net::rad_enabled()) {
      dsLog::debug("Sending RAD_SESSION_DHCP_END for %s (%s)", ip.s_addr, mac.s_addr);
      RADIUS_Net::send_acct_message(RAD_SESSION_DHCP_END, mac.s_addr, ip.s_addr);
    }
#endif
}

void IPAllocator::ip_invalidate(const MACAddress& mac, const IPAddress& ip){
  MACAddress badmac((uint64_t)0);
  record_lease(badmac, ip, "invalidated");
}
