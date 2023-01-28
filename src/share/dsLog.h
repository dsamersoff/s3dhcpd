#ifndef dsLog_h
#define dsLog_h

#include <time.h>

#include <iostream>

#include "dsSmartException.h"
#include "dsForm.h"
#include "dsApprc.h"
// #include "dsMutex.h"

#define MAX_LOG_COMPONENT_STACK 10

DECLARE_EXCEPTION(dsLog);

namespace libdms5 {

  // This manager coupled to dsApprc class and uses it's machinery
  // to store per-component verbosity. Unknown components just pushed with maximum verbosity
  class dsLogComponentManager {
    friend class dsLog;

    const char *_current_name;
    int _current_verbosity;

    const char *_stack[MAX_LOG_COMPONENT_STACK];
    int _stack_top;

    dsLogComponentManager() {
      _current_name = "sys";
      _current_verbosity = 99;
      _stack[0] = _current_name;
      _stack_top = 0;
    }

    int verbosity(const char *name) {
      char new_name[strlen(name) + 5];
      strcpy(new_name,"log-");
      strcat(new_name,name);
      return dsApprc::global_rc()->getint(new_name, 99);
    }

    void push_component(const char *name) {
      if (_stack_top == MAX_LOG_COMPONENT_STACK-1) {
        return;
      }
      _current_name = name;
      _current_verbosity = verbosity(name);
      _stack_top += 1;
      _stack[_stack_top] = _current_name;
    }

    void pop_component() {
      if (_stack_top == 0) {
        return;
      }
      _stack_top -= 1;
      _current_name = _stack[_stack_top];
      _current_verbosity = verbosity(_current_name);
    }

    const char* current_name(){ return _current_name; }
    int current_verbosity(){ return _current_verbosity; }
  };


  class dsLog {
      static dsLogComponentManager _manager;
      static std::ostream* _out;

      // Fixed prefix. Be warned it get overwritten on each call of set_prefix.
      static char *_prefix;

      // static libdms5::dsMutex _log_mutex;
      static void log_write(std::ostream& os, int flags, int priority, const char *format, va_list ap);
      static void write_ts(std::ostream& os);

      static void eol(std::ostream& os) {
        os << '\n'; os.flush();
      }


  public:

      static int current_verbosity() {
        return _manager.current_verbosity();
      }

      static void push_component(const char *name) {
        _manager.push_component(name);
      }

      static void pop_component() {
        _manager.pop_component();
      }

      static void set_output(std::ostream* os) {
        _out = os;
      }

      static void set_prefix(const char *prefix) {
        if (_prefix != NULL) {
          clear_prefix();
        }
        _prefix = dsStrdup(prefix);
      }

      static void clear_prefix() {
        if (_prefix != NULL) {
          free(_prefix);
        }
        _prefix = NULL;
      }

      static void set_output(const char *filename) {
        if (strcmp(filename,"cerr") == 0) { _out = &std::cerr; return; }
        if (strcmp(filename,"cout") == 0) { _out = &std::cout; return; }

        static std::ofstream myfile(filename, std::ios::app);
        if (!myfile.is_open()) { error("Can't open log file '%s'", filename); return; }
        _out = &myfile;
      }

      static void dsl_assert(bool condition, const char *format, ...);
      static void guarantee(bool condition, const char *format, ...);

      static int error(const char *format, ...);
      static int warn(const char *format, ...);
      static int info(const char *format, ... );
      static int debug(const char *format, ... );
      static int print(const char *format, ... );
      static int print_start(const char *format, ... );
      static int print_add(const char *format, ... );
      static int print_eol(const char *format, ... );

  };

  class dsLogScope {
  public:
    dsLogScope(const char *name) {
       dsLog::push_component(name);
    }

    ~dsLogScope() {
      dsLog::pop_component();
    }
  };

  class dsLogPrefix {
  public:
    dsLogPrefix(const char *prefix) {
       dsLog::set_prefix(prefix);
    }

    ~dsLogPrefix() {
      dsLog::clear_prefix();
    }
  };

};

#endif
