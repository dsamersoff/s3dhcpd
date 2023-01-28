/*
 *
 *
 *
 */

#ifndef CLOCK_TOOLS_H
#define CLOCK_TOOLS_H

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#ifdef __APPLE__
inline int get_clock() {
  // TODO: call timebase_info only once
  mach_timebase_info_data_t timebase_info;
  mach_timebase_info(&timebase_info);

  const uint64_t tm = mach_absolute_time();
  const uint64_t now = (tm * timebase_info.numer) / timebase_info.denom;

  return now;
}
#else
inline int get_clock() {
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  return tp.tv_sec;
}
#endif

inline void safe_sleep(int secs, int nsecs) {
  struct timespec req, rem;
  req.tv_sec = secs;
  req.tv_nsec = nsecs;

  while( nanosleep(&req, &rem) < 0 ) {
    if (errno != EINTR || (rem.tv_sec == 0 && rem.tv_nsec == 0)) {
      break;
    }

    req.tv_sec = rem.tv_sec;
    req.tv_nsec = rem.tv_nsec;
  }
}

inline void safe_sleep(int secs){
  safe_sleep(secs,0);
}

#endif
