#ifndef CV_H_INCLUDED
#define CV_H_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct thread_wakeup {
  HANDLE event;
  struct thread_wakeup *next;
};

typedef HANDLE (*cv_event_get_fn)();
typedef void (*cv_event_return_fn)(HANDLE event);

struct condvar {
  CRITICAL_SECTION wakeup_lock;
  struct thread_wakeup *first_wakeup;
  struct thread_wakeup *last_wakeup;
  unsigned char alertable;
  cv_event_get_fn get_fn;
  cv_event_return_fn return_fn;
};

void cv_init(struct condvar* cv, unsigned char alertable, cv_event_get_fn get_fn, cv_event_return_fn return_fn);
void cv_destroy(struct condvar* cv);
void cv_wait(struct condvar* cv, CRITICAL_SECTION* cs);
void cv_signal(struct condvar* cv);
void cv_bcast(struct condvar* cv);

#endif
