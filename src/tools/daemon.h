#ifndef _DAEMON_H
#define _DAEMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <signal.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <utime.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>
#include <ucontext.h>
#include <dlfcn.h>

#include <dsSmartException.h>
#include <dsLog.h>

#ifndef PID_PATH
# define PID_PATH "/var/run"
#endif

DECLARE_EXCEPTION(Daemon);

#define DAEMON_NOPARENTSIG     0x01
#define DAEMON_NOCHILDSIG      0x02
#define DAEMON_NODETACH        0x04
#define DAEMON_NOCHDIR         0x08
#define DAEMON_KILLPREV        0x10
#define DAEMON_NOCRASHHANDLER  0x20

typedef void sigfunc_t(int);   /* for signal handlers */

class Daemon {

  static uint32_t _flags;

  static void write_pid_file(const char *prog_name);
  static sigfunc_t* set_signal(int signo, sigfunc_t *func);
  static void SIGINT_hdl(int sig);
  static void CRASH_hdl(int sig, siginfo_t* info, ucontext_t* uc);

  public:

    static void daemonize(const char *prog_name, int options);
    static void kill_running(const char *progname);
};

#endif
