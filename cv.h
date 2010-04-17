#ifndef CV_H_INCLUDED
#define CV_H_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct thread_wakeup {
  HANDLE event;
  struct thread_wakeup *next;
};

struct condvar {
  CRITICAL_SECTION wakeup_lock;
  struct thread_wakeup *first_wakeup;
  struct thread_wakeup *last_wakeup;
  unsigned char alertable;
};

void cv_init(struct condvar* cv, unsigned char alertable);
void cv_destroy(struct condvar* cv);
void cv_wait(struct condvar* cv, CRITICAL_SECTION* cs);
void cv_signal(struct condvar* cv);
void cv_bcast(struct condvar* cv);

#endif
