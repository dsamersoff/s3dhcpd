#ifndef _PF_H_
#define _PF_H_

#include "dsSmartException.h"
#include "dsLog.h"

#include "pftable.h"
#include "tools.h"

DECLARE_EXCEPTION(PF);

class PFTable {

  static bool _enabled;
  static int _fd;
  static const char *_device;
  static const char *_default_table;

public:
  // Low level
  static void add(const char *table, uint32_t ip_addr) {
    char pb[32];
    libdms5::dsLog::debug("PF: Adding %s to '%s'", ip_u2s(ip_addr,pb), table);
    if (pftable_add(_fd, table, ip_addr, 32) < 0) {
      libdms5::dsLog::error("PF: Error adding %s to '%s' (%m)", ip_u2s(ip_addr,pb), table);
    }
  }

  static void del(const char *table, uint32_t ip_addr) {
    char pb[32];
    libdms5::dsLog::debug("PF: Deleting %s from '%s'", ip_u2s(ip_addr,pb), table);
    if (pftable_del(_fd, table, ip_addr, 32) < 0) {
      libdms5::dsLog::error("PF: Error deleting %s from '%s' (%m)", ip_u2s(ip_addr,pb), table);
    }
  }

  static void flush(const char *table) {
    libdms5::dsLog::debug("PF: Flushing '%s'", table);
    if (pftable_flush(_fd, table) < 0) {
      libdms5::dsLog::error("PF: Error flushing '%s' (%m)", table);
    }
  }

  // High level
  static void add(const char *ip_addr) {
    add(_default_table, ip_s2u(ip_addr));
  }

  static void del(const char *ip_addr) {
    del(_default_table, ip_s2u(ip_addr));
  }

  static void add(uint32_t ip_addr) {
    add(_default_table,ip_addr);
  }

  static void del(uint32_t ip_addr) {
    del(_default_table, ip_addr);
  }

  static void flush() {
    flush(_default_table);
  }

  static bool pf_enabled(){ return _enabled; }
  static const char * default_table(){ return _default_table; }

  // factory
  static void initialize(const char *device, const char *table_name);
};


#endif
