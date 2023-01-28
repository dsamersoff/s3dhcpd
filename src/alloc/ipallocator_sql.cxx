/**
 *
 * IP Allocator with SQL(sqlite) backend
 *
 */

#include "dsApprc.h"
#include "dsLog.h"

#include "ipallocator_sql.h"
#include "tools.h"
#include "clock_tools.h"

#ifdef WITH_PF
# include "pf.h"
#endif

using namespace libdms5;

IPAllocatorSQL::IPAllocatorSQL() {
  _leases_db = NULL;
  _static_db = NULL;
  _static_get_query = NULL;

  prepare_databases();
}

IPAllocatorSQL::~IPAllocatorSQL() {
  if (_static_db) {
    sqlite3_close(_static_db);
  }
  if (_leases_db) {
    sqlite3_close(_leases_db);
  }
}

void IPAllocatorSQL::prepare_databases() {
  dsApprc *rc = dsApprc::global_rc();
  _static_db = NULL;

  dsLog::debug("Preparing database");

  // Prepare to get static assignment
  if (rc->has_key("database-static")) {
    const char *static_database = rc->getstring("database-static");
    _static_get_query = GRC_GET("database-static-query");
    if (sqlite3_open_v2(static_database, &_static_db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
      throw IPAllocatorException("Can't open database %s - %s", static_database, sqlite3_errmsg(_static_db));
    }

    // Validate user-supplied query
    // Make sure it doesn't have syntax errors and returns exactly two columns: ip_addr, inet_access
    // e.g.
    // {select ip_addr, inet_access from wifi_forge where mac_addr=? collate nocase}
    // TODO: keep pre-compiled query for better performance
    sqlite3_stmt *statement;

    if(sqlite3_prepare_v2(_static_db, _static_get_query, -1, &statement, 0) != SQLITE_OK) {
      sqlite3_finalize(statement);
      throw IPAllocatorException("Can't compile static alloc query {%s} - %s", _static_get_query, sqlite3_errmsg(_static_db));
    }

    if (sqlite3_column_count(statement) != 2) {
      sqlite3_finalize(statement);
      throw IPAllocatorException("Static query {%s} should return exactly two columns", _static_get_query);
    }

    sqlite3_finalize(statement);
  }

  // Manage lease database. It's madatory ever if you don't have a network for dynamic lease
  const char *leases_database = rc->getstring("database-leases");
  // leases database doesn't exist create it
  if (access(leases_database,F_OK) < 0) {
    sqlite3 *leases_db = NULL;
    if (sqlite3_open_v2(leases_database, &leases_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
      throw IPAllocatorException("Can't open database %s - %s", leases_database, sqlite3_errmsg(leases_db));
    }

    // create a database schema
    // id is an alias for sqlite3 rowid, don't require additional care

    char *sql_error_msg = 0;
    int rc = sqlite3_exec(leases_db,
       "CREATE TABLE dhcp_leases (id integer primary key, mac varchar(20), ip varchar(20), \
                          ts integer, expire integer, auth_info varchar(64));",
        NULL /*no callback*/, NULL, &sql_error_msg);

    if (rc != SQLITE_OK) {
      throw IPAllocatorException("Can't create database schema %s - %s", leases_database, sqlite3_errmsg(leases_db));
    }

    // Assume that we always can create indices on empty database
    sqlite3_exec(leases_db,
        "CREATE INDEX dhcp_leases_mac_idx on dhcp_leases(mac,ts);",
        NULL /*no callback*/, NULL, &sql_error_msg);

    sqlite3_exec(leases_db,
        "CREATE INDEX dhcp_leases_ts_idx on dhcp_leases(ts);",
        NULL /*no callback*/, NULL, &sql_error_msg);

    // The only lease record per IP allowed
    sqlite3_exec(leases_db,
        "CREATE UNIQUE INDEX dhcp_leases_ip_idx on dhcp_leases(ip);",
        NULL /*no callback*/, NULL, &sql_error_msg);

    sqlite3_close(leases_db);
  }

  if (sqlite3_open_v2(leases_database, &_leases_db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
    throw IPAllocatorException("Can't open database %s - %s", leases_database, sqlite3_errmsg(_leases_db));
  }

  {
    // We have to keep current leases across a server restart,
    // But handle the situation where network range is changed.

    // Populate database (slow, but we do it on start only):
    // 1. TODO: Remove all leases that don't belong to any of configured networks
    //   we check that IP address belong to one of configured netowrk at ACK time,
    //   so stale IP addresses is not harmful but can pollute a database
    // 2. Add IP address from previousely unknown networks

    const char query[] = "insert into dhcp_leases (mac, ip, ts, expire, auth_info) values ('', ?, 0, 0, '-')";
    sqlite3_stmt *statement;

    if(sqlite3_prepare_v2(_leases_db, query, -1, &statement, 0) != SQLITE_OK) {
      throw IPAllocatorException("Can't compile a query {%s} - %s", query, sqlite3_errmsg(_leases_db));
    }

    Subnet *sub = Subnets::subnets();
    while(sub != NULL) {
      // subnet don't have dynamic range
      if (! sub->has_dynamic_range()) {
        sub = sub->next;
        continue;
      }

      uint32_t start_range = ip_isolate_addr(sub->netmask.u_addr, sub->range_start.u_addr);
      uint32_t end_range = ip_isolate_addr(sub->netmask.u_addr, sub->range_end.u_addr);

      dsLog::debug("Dynamic ip range %d %d", start_range, end_range);

      for (uint32_t new_ip = start_range; new_ip < end_range; ++new_ip) {
        sqlite3_reset(statement);
        IPAddress scip = sub->compose_ip(new_ip);
        dsLog::debug("Populating %s", scip.s_addr);
        if (sqlite3_bind_text(statement, 1, scip.s_addr, strlen(scip.s_addr), NULL) != SQLITE_OK) {
          sqlite3_finalize(statement);
          throw IPAllocatorException("Can't bind ip %s - %s", scip.s_addr, sqlite3_errmsg(_leases_db));
        }
        if (sqlite3_step(statement) != SQLITE_DONE) {
          if (sqlite3_errcode(_leases_db) != SQLITE_CONSTRAINT) {
            // Ignore UNIQUE constraint violation
            sqlite3_finalize(statement);
            throw IPAllocatorException("Can't populate lease for %s - %s", scip.s_addr, sqlite3_errmsg(_leases_db));
          }
        }
      }
      sub = sub->next;
    }
    sqlite3_finalize(statement);
  }

#ifdef WITH_PF
  // If compiled with PF support pre-populat PF table with already allocated static addresses
  // TODO: inet_access field is ignored for now
  if (PFTable::pf_enabled()) {
    dsLog::debug("Populating static addresses to PF table");

    const char query[] = "select ip from dhcp_leases where auth_info = 'static'";
    sqlite3_stmt *statement;

    if(sqlite3_prepare_v2(_leases_db, query, -1, &statement, 0) != SQLITE_OK) {
      throw IPAllocatorException("Can't compile a query {%s} - %s", query, sqlite3_errmsg(_leases_db));
    }

    while(true) {
      int rc = sqlite3_step(statement);
      if (rc == SQLITE_DONE) {
        break;
      }

      if (rc != SQLITE_ROW) {
        dsLog::error("Can't pre-populate static addresses. (ERROR: %d)", rc);
        break;
      }

      IPAddress ip = IPAddress((char*)sqlite3_column_text(statement, 0));
      dsLog::debug("Adding static %s to pf admitted list", ip.s_addr);
      PFTable::add(ip.u_addr);
    }

    sqlite3_finalize(statement);
  }
#endif

}

/**
 * Get static allocation information. User supplied database could be used
 */
bool IPAllocatorSQL::get_static_ip(const MACAddress& mac, IPAddress& ip, int *inet_access) {
  sqlite3_stmt *statement;
  bool ip_found = false;

  // Attempt to get static IP from rc file first
  if (IPAllocator::get_static_ip(mac, ip, inet_access)) {
    return true;
  }

  if (_static_db == NULL) {
    // static database is not configured just return false
    return false;
  }

  if(sqlite3_prepare_v2(_static_db, _static_get_query, -1, &statement, 0) != SQLITE_OK) {
    throw IPAllocatorException("Can't compile a query {%s} - %s", _static_get_query, sqlite3_errmsg(_static_db));
  }

  if (sqlite3_bind_text(statement, 1, mac.s_addr, strlen(mac.s_addr), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind mac parameter %s - %s", mac.s_addr, sqlite3_errmsg(_static_db));
  }

  int rc = sqlite3_step(statement);
  if (rc == SQLITE_ROW) {
    ip = IPAddress((char*)sqlite3_column_text(statement, 0));
    ip_found = true;

    char *str_access = (char*)sqlite3_column_text(statement, 1);
    *inet_access = (*str_access == 'Y' || *str_access == 'y') ? 1 : 0;
  }

  // query should return the only row  but it migt not be true
  // first returned record is used

  if (rc != SQLITE_DONE && sqlite3_step(statement) != SQLITE_DONE) {
    dsLog::error("User query return more than one row. Only first row used");
  }

  sqlite3_finalize(statement);
  return ip_found;
}

/**
 * Attempt to return last known address ever if it's already expired
 */

bool IPAllocatorSQL::get_leased_ip(const MACAddress& mac, IPAddress& ip) {
  sqlite3_stmt *statement;
  bool ip_found = false;

  const char query[] = "select ip from dhcp_leases where mac = ? and auth_info <> 'excluded' limit 1";

  if(sqlite3_prepare_v2(_leases_db, query, -1, &statement, 0) != SQLITE_OK) {
    throw IPAllocatorException("Can't compile a query {%s} - %s", query, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_text(statement, 1, mac.s_addr, strlen(mac.s_addr), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind mac parameter %s - %s", mac.s_addr, sqlite3_errmsg(_leases_db));
  }

  if ( sqlite3_step(statement) == SQLITE_ROW) {
    ip = IPAddress((char*)sqlite3_column_text(statement, 0));
    ip_found = true;
  }

  sqlite3_finalize(statement);
  return ip_found;
}

bool IPAllocatorSQL::get_new_ip(const MACAddress& mac, IPAddress& ip) {
  sqlite3_stmt *statement;
  bool ip_found = false;
  int now_ts = get_clock();

  const char query[] = "select ip from dhcp_leases where expire < ? and auth_info <> 'excluded' order by expire limit 1";

  if(sqlite3_prepare_v2(_leases_db, query, -1, &statement, 0) != SQLITE_OK) {
    throw IPAllocatorException("Can't compile a query {%s} - %s", query, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_int(statement, 1, now_ts) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind ts parameter %d - %s", now_ts, sqlite3_errmsg(_leases_db));
  }

  if ( sqlite3_step(statement) == SQLITE_ROW) {
    // cast from unsigned char to call correct constructor
    ip = IPAddress((char *)sqlite3_column_text(statement, 0));
    ip_found = true;
  }

  sqlite3_finalize(statement);
  return ip_found;
}

/**
 * Record allocated lease. IP address have to be pre-populated to database,
 * Delete any authentication data
 */
void IPAllocatorSQL::record_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
  if (do_update_lease(mac, ip, auth_info) != 1) {
    dsLog::error("Lease for %s is not updated", ip.s_addr);
  }
}

/**
 * Store allocated static IP to leases database to be able to track expiration and for
 * accounting purpose. As user database can be huge we have no reason to copy all ip addresses
 * so store only allocated one.
 * We trying to do update but if update fails - try to insert
 */
void IPAllocatorSQL::record_static_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
  // Try to update existing record first
  if (do_update_lease(mac, ip, auth_info) != 1) {
    // No records updated, try to insert new record
    if (do_create_lease(mac, ip, auth_info) != 1) {
      dsLog::error("Static lease for %s is not recorded", ip.s_addr);
    }
  }
}

// Update lease record to db
int IPAllocatorSQL::do_update_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {
  sqlite3_stmt *statement;
  const char *query;
  int now_ts = get_clock();
  int expire = now_ts + Subnets::globalnet()->lease_time;
  int position = 1;

  dsLog::debug("Updating lease record for %s auth_info: %s", ip.s_addr, (auth_info == NULL) ? "Not provided" : auth_info);

  query = "update dhcp_leases set mac = ?, ts = ?, expire = ? where ip = ?";
  if (auth_info != NULL) {
    // Auth info provided, update existing information
    query = "update dhcp_leases set mac = ?, ts = ?, expire = ?, auth_info = ? where ip = ?";
  }

  if(sqlite3_prepare_v2(_leases_db, query, -1, &statement, 0) != SQLITE_OK) {
    throw IPAllocatorException("Can't compile a query {%s} - %s", query, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_text(statement, position++, mac.s_addr, strlen(mac.s_addr), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind mac parameter at %d %s - %s", (position-1), mac.s_addr, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_int(statement, position++, now_ts) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind ts parameter at %d %d - %s", (position-1), now_ts, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_int(statement, position++, expire) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind expire parameter %d %d - %s", (position-1), expire, sqlite3_errmsg(_leases_db));
  }

  if (auth_info != NULL) {
    if (sqlite3_bind_text(statement, position++, auth_info, strlen(auth_info), NULL) != SQLITE_OK) {
      sqlite3_finalize(statement);
      throw IPAllocatorException("Can't bind auth_info parameter %d %s - %s", (position-1), auth_info, sqlite3_errmsg(_leases_db));
    }
  }

  if (sqlite3_bind_text(statement, position++, ip.s_addr, strlen(ip.s_addr), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind ip parameter at %d %s - %s", (position-1), ip.s_addr, sqlite3_errmsg(_leases_db));
  }

  // Actually run the query
  if (sqlite3_step(statement) != SQLITE_DONE) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't update lease for %s - %s", ip.s_addr, sqlite3_errmsg(_leases_db));
  }
  sqlite3_finalize(statement);
  return sqlite3_changes(_leases_db);
}

// Create new lease record in database
int IPAllocatorSQL::do_create_lease(const MACAddress& mac, const IPAddress& ip, const char *auth_info) {

  sqlite3_stmt *statement;
  const char *query;
  int now_ts = get_clock();
  int expire = now_ts + Subnets::globalnet()->lease_time;
  int position = 1;

  dsLog::debug("Creating new lease record for %s", ip.s_addr);
  dsLog::guarantee(auth_info != NULL, "Auth info can't be NULL at this point");

  query = "insert into dhcp_leases (mac, ip, ts, expire, auth_info) values (?, ?, ?, ?, ?)";

  if(sqlite3_prepare_v2(_leases_db, query, -1, &statement, 0) != SQLITE_OK) {
    throw IPAllocatorException("Can't compile a query {%s} - %s", query, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_text(statement, position++, mac.s_addr, strlen(mac.s_addr), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind mac parameter at %d %s - %s", (position-1), mac.s_addr, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_text(statement, position++, ip.s_addr, strlen(ip.s_addr), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind ip parameter at %d %s - %s", (position-1), ip.s_addr, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_int(statement, position++, now_ts) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind ts parameter at %d %d - %s", (position-1), now_ts, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_int(statement, position++, expire) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind expire parameter %d %d - %s", (position-1), expire, sqlite3_errmsg(_leases_db));
  }

  if (sqlite3_bind_text(statement, position++, auth_info, strlen(auth_info), NULL) != SQLITE_OK) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't bind auth_info parameter %d %s - %s", (position-1), auth_info, sqlite3_errmsg(_leases_db));
  }

  // Actually run the query
  if (sqlite3_step(statement) != SQLITE_DONE) {
    sqlite3_finalize(statement);
    throw IPAllocatorException("Can't update lease for %s - %s", ip.s_addr, sqlite3_errmsg(_leases_db));
  }
  sqlite3_finalize(statement);
  return sqlite3_changes(_leases_db);
}
