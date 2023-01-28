#include <iostream>

#include "dsLog.h"
#include "dsForm.h"

using namespace std;
using namespace libdms5;

dsLogComponentManager dsLog::_manager;
ostream* dsLog::_out = &cerr;
char *dsLog::_prefix = NULL;

// dsMutex dsLog::_log_mutex;

#define WITH_BANNER 0x1
#define WITH_EOL    0x2

void dsLog::write_ts(ostream& os) {
  time_t rawtime;
  struct tm *info;
  char buf[30];

  //TODO: Use clock_gettime instead
  //  struct timespec ts;
  //  clock_gettime(CLOCK_MONOTONIC, &ts);

  time( &rawtime );
  info = localtime( &rawtime );
  strftime(buf, 30, "%F %T", info);
  dsForm::form(os, "%s ", buf);
}


void dsLog::log_write(ostream& os, int flags, int priority, const char *format, va_list ap){
  if (priority == 0 || _manager.current_verbosity() >= priority) {
    // _log_mutex.lock();
    if (flags & WITH_BANNER) {
      //  dsForm::form(os,"%x:",pthread_self());
      write_ts(os);
      if (_prefix != NULL) {
        dsForm::form(os,"[%s]", _prefix);
      }

      if (priority == 0) {
        // Shorten banner for print command
          dsForm::form(os,"%s: ",  _manager.current_name());
      }
      else {
        dsForm::form(os,"%s(%d,%d): ", _manager.current_name(), _manager.current_verbosity(), priority);
      }
    }

    dsForm::vform(os, format, ap);

    // Don't flush uncomplete string
    if (flags & WITH_EOL){
      dsLog::eol(os);
    }
    // _log_mutex.unlock();
  }
}

void dsLog::dsl_assert(bool condition, const char *format, ...) {
#ifdef ENABLE_ASSERTS
  if (!condition) {
    va_list ap;
    va_start(ap, format);
      dsForm::form(*_out,"ASSERT_FAILED: ");
      dsForm::vform(*_out, format, ap);
      dsLog::eol(*_out);
    va_end(ap);

    throw dsLogException("Assert failed");
  }
#endif
}

void dsLog::guarantee(bool condition, const char *format, ...) {
  if (!condition) {
    va_list ap;
    va_start(ap, format);
      dsForm::form(*_out,"ASSERT_FAILED: ");
      dsForm::vform(*_out, format, ap);
      dsLog::eol(*_out);
    va_end(ap);

    throw dsLogException("Guarantee failed");
  }
}

int dsLog::error(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_BANNER| WITH_EOL, 1, format, ap);
  va_end(ap);
  return -1;
}

int dsLog::warn(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_BANNER | WITH_EOL, 2, format, ap);
  va_end(ap);
  return 1;
}

int dsLog::info(const char *format, ... ) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_BANNER | WITH_EOL, 3, format, ap);
  va_end(ap);
  return 0;
}

int dsLog::debug(const char *format, ... ) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_BANNER| WITH_EOL, 4, format, ap);
  va_end(ap);
  return 0;
}

int dsLog::print(const char *format, ... ) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_BANNER| WITH_EOL, 0, format, ap);
  va_end(ap);
  return 0;
}

int dsLog::print_start(const char *format, ... ) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_BANNER, 0, format, ap);
  va_end(ap);
  return 0;
}

int dsLog::print_add(const char *format, ... ) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, 0, 0, format, ap);
  va_end(ap);
  return 0;
}

int dsLog::print_eol(const char *format, ... ) {
  va_list ap;
  va_start(ap, format);
    log_write(*_out, WITH_EOL, 0, format, ap);
  va_end(ap);
  return 0;
}
