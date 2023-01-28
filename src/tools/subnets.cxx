/**
 *
 *
 *
 *
 *
 */

#include "dsApprc.h"
#include "dsLog.h"

#include "subnets.h"

using namespace libdms5;

Subnet *Subnets::_subnets = NULL;
Subnet *Subnets::_global_net = NULL;

// Read and cache subnet information from global rc
void Subnets::load() {
  dsApprc *rc = dsApprc::global_rc();
  int walk_size = rc->walk_size();

  _global_net = new Subnet();
  _global_net->domain_name = rc->getstring("global-domain-name");
  _global_net->addr = IPAddress(rc->getstring("global-server-addr"));
  _global_net->netmask = IPAddress(rc->getstring("global-server-netmask"));
  // lease time - maximum time client may own IP address
  _global_net->lease_time = rc->getint("global-lease-time");
  // renew time - time to check lease validity. Have to be less than lease time
  _global_net->renew_time = rc->getint("global-renew-time");

  if (_global_net->renew_time > _global_net->lease_time) {
    throw SubnetException("Renew time %d should be less or equal than lease_time %d",
                          _global_net->renew_time, _global_net->lease_time);
  }

  uint32_t bcast_addr = get_ip_broadcast(_global_net->u_addr(), _global_net->netmask.u_addr);
  _global_net->broadcast = IPAddress(bcast_addr);

  rc->push("subnet");
  Subnet *prev = NULL;

  for(int i = 0; i < walk_size; ++i) {
    const char *key = rc->walk(i);
    // extarct subnet name from key, key should be subnet-name-addr
    if (key && dsStartsWith(key,"subnet-") && dsEndsWith(key,"-addr")) {
      Subnet *sub = new Subnet();
      int len = strlen(key);
      sub->name = dsStrndup(key + 7, len - 7 - 5);

      rc->push(sub->name);
      sub->addr = IPAddress(rc->getstring("-addr"));

      dsLog::debug("Found subnet '%s' '%s' (%x)", sub->name, sub->s_addr(), sub->u_addr());

      sub->netmask = IPAddress(rc->getstring("-netmask"));;
      sub->broadcast = IPAddress(get_ip_broadcast(sub->u_addr(), sub->netmask.u_addr));

      sub->gateway = IPAddress(rc->getstring("-gateway"));;

      // One nameserver is required. Attempt to set it to either server or subnet one
      sub->nameserver1 = IPAddress(rc->getstring("-nameserver1", rc->getstring("global-nameserver1")));

      // Try different nameserver options for nameserver2
      // 1. Per-subnet nameserver 2
      // 2. Global nameserver 2
      // 3. No Nameserever 2
      if (rc->has_key("-nameserver2")) {
        sub->nameserver2 = IPAddress(rc->getstring("-nameserver2"));
      }
      else {
        if (rc->has_key("global-nameserver2")) {
          sub->nameserver2 = IPAddress(rc->getstring("global-nameserver2"));
        }
      }

      // It's not possible to have range-start without range-end
      if (rc->has_key("-range-start")) {
        sub->range_start = IPAddress(rc->getstring("-range-start"));
        sub->range_end = IPAddress(rc->getstring("-range-end"));

        if (! sub->myaddress(sub->range_start) || !sub->myaddress(sub->range_end)) {
          throw SubnetException("IP addresses range is out of subnet");
        }
      }

      // Set lease an renew time to either global or per-subnet value
      // TODO: Support per subnet lease-time. There is a problem passing it to allocator
      //
      // sub->lease_time = rc->getint("-lease-time", rc->getint("global-lease-time"));
      // sub->renew_time = rc->getint("-renew-time", rc->getint("global-renew-time"));

      rc->pop();

      //Add the option into the packet
      if (_subnets == NULL) {
        _subnets = sub;
      }
      else {
        // Lint reports false null pointer dereference here
        prev->next = sub;
      }
      prev = sub;
    }
  }

  rc->pop();
}


Subnet *Subnets::mysubnet(const IPAddress& ip) {
  Subnet *sub = _subnets;
  while(sub != NULL) {
    if (sub->myaddress(ip)) {
      return sub;
    }
    sub = sub->next;
  }
  return NULL;
}
