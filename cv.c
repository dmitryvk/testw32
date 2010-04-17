#include <stdio.h>
#include "cv.h"

static HANDLE cv_default_event_get_fn()
{
  return CreateEvent(NULL, FALSE, FALSE, NULL);
}

static void cv_default_event_return_fn(HANDLE event)
{
  CloseHandle(event);
}

void cv_init(struct condvar* cv, unsigned char alertable, cv_event_get_fn get_fn, cv_event_return_fn return_fn)
{
  InitializeCriticalSection(&cv->wakeup_lock);
  cv->first_wakeup = NULL;
  cv->last_wakeup = NULL;
  cv->alertable = alertable;
  cv->get_fn = get_fn ? get_fn : cv_default_event_get_fn;
  cv->return_fn = return_fn ? return_fn : cv_default_event_return_fn;
}

void cv_destroy(struct condvar* cv)
{
  DeleteCriticalSection(&cv->wakeup_lock);
}

void cv_print_wakeups(struct condvar* cv)
{
  struct thread_wakeup* w;
  int first = 1;
  fprintf(stderr, "For cv 0x%p, wakeup chain is: ", cv);
  w = cv->first_wakeup;
  while (w)
  {
    if (!first)
      fprintf(stderr, ", ");
    first = 0;
    fprintf(stderr, "0x%p", w);
    w = w->next;
  }
  fprintf(stderr, "\n");
}

void cv_wakeup_add(struct condvar* cv, struct thread_wakeup* w)
{
  w->event = cv->get_fn();
  w->next = NULL;
  EnterCriticalSection(&cv->wakeup_lock);
  if (cv->last_wakeup != NULL)
  {
    cv->last_wakeup->next = w;
    cv->last_wakeup = w;
  }
  else
  {
    cv->first_wakeup = w;
    cv->last_wakeup = w;
  }
  //fprintf(stderr, "added wakeup:\n");
  //cv_print_wakeups(cv);
  LeaveCriticalSection(&cv->wakeup_lock);
}

void cv_wait(struct condvar* cv, CRITICAL_SECTION* cs)
{
  struct thread_wakeup w;
  cv_wakeup_add(cv, &w);
  LeaveCriticalSection(cs);
  if (cv->alertable) {
    while (WaitForSingleObjectEx(w.event, INFINITE, TRUE) == WAIT_IO_COMPLETION);
  } else {
    WaitForSingleObject(w.event, INFINITE);
  }
  cv->return_fn(w.event);
  EnterCriticalSection(cs);
}

void cv_signal(struct condvar* cv)
{
  struct thread_wakeup * w;
  EnterCriticalSection(&cv->wakeup_lock);
  w = cv->first_wakeup;
  if (!w) return;
  cv->first_wakeup = w->next;
  if (!cv->first_wakeup) cv->last_wakeup = NULL;
  SetEvent(w->event);
  LeaveCriticalSection(&cv->wakeup_lock);
}

void cv_bcast(struct condvar* cv)
{
  int count = 0;
  EnterCriticalSection(&cv->wakeup_lock);
  while (cv->first_wakeup)
  {
    struct thread_wakeup * w = cv->first_wakeup;
    cv->first_wakeup = w->next;
    SetEvent(w->event);
    ++count;
  }
  cv->last_wakeup = NULL;
  LeaveCriticalSection(&cv->wakeup_lock);
  //fprintf(stderr, "bcast: woke up %d threads\n", count);
}
