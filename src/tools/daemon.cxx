/*
 *
 *
 *
 */

#include "daemon.h"
#include "clock_tools.h"

#ifndef PID_PATH
# define PID_PATH "/var/run"
#endif

extern const char *VERSION();

using namespace libdms5;

uint32_t Daemon::_flags = 0;

void Daemon::kill_running(const char *prog_name) {
  FILE *pidf;
  long  pid;
  char pidname[PATH_MAX];
  sprintf(pidname,"%s/%s.pid",PID_PATH, prog_name);

/* first at all, check pid file */
   if (access(pidname,0) != -1){
      if (!(pidf = fopen(pidname,"r"))) {
           throw DaemonException("Can't read PID file '%s' (%m)", pidname);
      }
      fscanf(pidf,"%ld",&pid);
      fclose(pidf);
      
      if (pid > 1) { // paranoyya
        while(kill(pid,SIGTERM) != -1) {
          safe_sleep(1);
        }
      }

      if (kill(pid,0) != -1) {
         throw DaemonException("Can't kill %s pid:%ld, (%m)", prog_name, (long) pid);
      }
   }
}

void Daemon::write_pid_file(const char *prog_name){
  FILE *pidf;
  long  pid;

  char pidname[PATH_MAX];
  sprintf(pidname,"%s/%s.pid",PID_PATH,prog_name);

/* first at all, check pid file */
  if (access(pidname,0) != -1) {
    if (!(pidf = fopen(pidname,"r"))) {
      throw DaemonException("Can't read PID file '%s' (%m)", pidname);
    }

    fscanf(pidf,"%ld",&pid);
    fclose(pidf);

    if ((_flags & DAEMON_KILLPREV)) {
      if (pid > 1) { // paranoyya
        while(kill(pid,SIGTERM) != -1){
          safe_sleep(1);
        }
      }
    }

    if (kill(pid,0) != -1) {
      throw DaemonException("%s alredy run with pid '%ld'", prog_name, (long) pid);
    }
  }
 /* create new pid file */
  if (!(pidf = fopen(pidname,"w"))) {
    throw DaemonException("Can't create PID file '%s' (%m)", pidname);
  }

  fprintf(pidf,"%ld",(long) getpid());
  fclose(pidf);
}

sigfunc_t* Daemon::set_signal(int signo, sigfunc_t *func) {
  struct sigaction  act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (signo == SIGALRM) {
#ifdef  SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;   /* SunOS */
#endif
  }
  else {
#ifdef  SA_RESTART
    act.sa_flags |= SA_RESTART;   /* SVR4, 44BSD */
#endif
  }

  if (sigaction(signo, &act, &oact) < 0) {
    return(SIG_ERR);
  }

  return(oact.sa_handler);
}

void Daemon::SIGINT_hdl(int sig){
  if (sig != SIGHUP) {
    exit(-1);
  }
}

// Basic crash handler that write stacktrace directly to log
// TODO: Full handler for Darwin
// TODO: Support arm and aarch64 platforms
#if defined (__APPLE__)
void Daemon::CRASH_hdl(int sig, siginfo_t* info, ucontext_t* uc) {
  // Restore default handler
  set_signal(SIGABRT, NULL);

  if (uc == NULL) {
    // A bit of paranoja
    ::abort();
  }

  void *ip = (void *)(uc->uc_mcontext->__es.__faultvaddr);

  dsLog::print("#");
  dsLog::print("# An unexpected error has been detected:");
  dsLog::print("#");
  dsLog::print("# SIGNAL %d at ip=0x%p, pid=%ld\n", sig,  ip, (long) getpid());
  dsLog::print("#");
  dsLog::print("# %s", VERSION());
  dsLog::print("#");

  ::abort();
}

#else
void Daemon::CRASH_hdl(int sig, siginfo_t* info, ucontext_t* uc) {
  // Restore default handler
  set_signal(SIGABRT, NULL);

  if (uc == NULL) {
    // A bit of paranoja
    ::abort();
  }

  /* extract key registers */
  void *ip = NULL, *sp = NULL, **bp = NULL;
  Dl_info dlinfo;

  #if defined(Linux)
    ip  = (void*) uc->uc_mcontext.gregs[REG_EIP];
    sp = (void*)  uc->uc_mcontext.gregs[REG_ESP];
    bp = (void**) uc->uc_mcontext.gregs[REG_EBP];
  #endif

  #if defined(FreeBSD) 
    ip  = (void*) uc->uc_mcontext.mc_rip;
    sp = (void*) uc->uc_mcontext.mc_rsp;
    bp = (void**) uc->uc_mcontext.mc_rbp;
  #endif

  dsLog::print("#");
  dsLog::print("# An unexpected error has been detected:");
  dsLog::print("#");
  dsLog::print("# SIGNAL %d at ip=0x%p, pid=%ld\n", sig,  ip, (long) getpid());
  dsLog::print("#");
  dsLog::print("# %s", VERSION());
  dsLog::print("#");
  dsLog::print("# Problematic frame:");

  if (dladdr(ip, &dlinfo)) {
    dsLog::print("#       %p: <%s+%p> %s\n",bp, dlinfo.dli_fname, ip, dlinfo.dli_sname);
  }

  dsLog::print("# Stack: sp=%p\n", sp);

  int cnt = 0;

  while(bp && ip && (cnt++ < 100) ) {
    if(!dladdr(ip, &dlinfo))
      break;

    dsLog::print("# %02d: %p: <%s+%p>",cnt, bp, dlinfo.dli_fname, ip);

    if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main")) break;

    ip = bp[1];
    bp = (void**)bp[0];
  }

  ::abort();
}
#endif

void Daemon::daemonize(const char *prog_name, int options) {
  int res;
  _flags = options;

  if ((options & DAEMON_NOPARENTSIG) == 0) {
    set_signal(SIGTTOU, SIG_IGN);
    set_signal(SIGTTIN, SIG_IGN);
    set_signal(SIGTSTP, SIG_IGN);
    set_signal(SIGTRAP, SIG_IGN);

    set_signal(SIGTERM, SIGINT_hdl);
    set_signal(SIGINT, SIGINT_hdl);
    set_signal(SIGHUP, SIGINT_hdl);
  }

  if ((options & DAEMON_NOCRASHHANDLER) == 0) {
    set_signal(SIGILL,  (sigfunc_t *) CRASH_hdl);
    set_signal(SIGSEGV, (sigfunc_t *) CRASH_hdl);
    set_signal(SIGBUS, (sigfunc_t *) CRASH_hdl);
    set_signal(SIGFPE, (sigfunc_t *) CRASH_hdl);
  }

  if ((options & DAEMON_NODETACH) == 0) {
    if ((res = fork()) < 0) {
      throw DaemonException("Fork error (%m)");
    }

    if (res >  0) {   /* parent must die */
      exit(0);
    }

    write_pid_file(prog_name);

    setsid();

    // Сlose first 64 file descriptors
    // TODO: do it better
    for (int i = 0; i < 64; ++i) {
      close(i);
    }

    if ((options & DAEMON_NOCHDIR) == 0) {
      chdir("/var/tmp");
    }
  }
}
