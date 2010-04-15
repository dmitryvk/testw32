#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

typedef enum State { Initial, ServerRequest, ClientResponse } State;

struct thread_wakeup {
  HANDLE event;
  struct thread_wakeup *next;
};

struct condvar {
  CRITICAL_SECTION wakeup_lock;
  struct thread_wakeup *first_wakeup;
  struct thread_wakeup *last_wakeup;
};

void cv_init(struct condvar* cv);
void cv_destroy(struct condvar* cv);
void cv_wait(struct condvar* cv, CRITICAL_SECTION* cs);
void cv_signal(struct condvar* cv);
void cv_bcast(struct condvar* cv);

typedef struct ThreadData {
	CRITICAL_SECTION m;
	struct condvar cv;
	State state;
	int server_started;
	int arg;
} ThreadData;

DWORD WINAPI server_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int i;
	EnterCriticalSection(&data->m);
	printf("Server started in %d\n", (int)GetCurrentThreadId());
	for (i = 0; i < 100; ++i) {
		data->state = ServerRequest;
		data->arg = i;
		printf("%d: Server send %d\n", (int)GetCurrentThreadId(), data->arg);
		cv_bcast(&data->cv);
		while (data->state != ClientResponse)
			cv_wait(&data->cv, &data->m);
		printf("%d: Server received %d\n", (int)GetCurrentThreadId(), data->arg);
	}
	data->state = ServerRequest;
	data->arg = -1;
	printf("Sending exit to client\n");
	cv_bcast(&data->cv);
	LeaveCriticalSection(&data->m);
	return 0;
}

DWORD WINAPI client_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int looping = 1;
	EnterCriticalSection(&data->m);
	printf("Client started in %d\n", (int)GetCurrentThreadId());
	while (looping) {
		while (data->state != ServerRequest)
			cv_wait(&data->cv, &data->m);
		printf("%d: Client received: %d\n", (int)GetCurrentThreadId(), data->arg);
		if (data->arg == -1) {
			printf("%d: Client exiting\n", (int)GetCurrentThreadId());
			looping = 0;
		} else {
			data->arg = data->arg * data->arg;
			data->state = ClientResponse;
			printf("%d: Client sent %d\n", (int)GetCurrentThreadId(), data->arg);
			cv_bcast(&data->cv);
		}
	}
	LeaveCriticalSection(&data->m);
	return 0;
}

int main(int argc, char *argv[])
{
	ThreadData d;
	HANDLE hserver, hclient, h1, h2, h3;
  InitializeCriticalSection(&d.m);
  cv_init(&d.cv);
	d.state = Initial;
	
	hserver = CreateThread(NULL, 0, server_thread, (void*)&d, 0, NULL);
	hclient = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h1 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h2 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h3 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	
	WaitForSingleObject(hserver, INFINITE);
  fprintf(stderr, "ok1\n");
	WaitForSingleObject(hclient, INFINITE);
  fprintf(stderr, "ok2\n");
  EnterCriticalSection(&d.m);
  cv_bcast(&d.cv);
  LeaveCriticalSection(&d.m);
	WaitForSingleObject(h1, INFINITE);
  fprintf(stderr, "ok3\n");
	WaitForSingleObject(h2, INFINITE);
  fprintf(stderr, "ok4\n");
	WaitForSingleObject(h3, INFINITE);
  fprintf(stderr, "ok5\n");
  
  cv_destroy(&d.cv);
  DeleteCriticalSection(&d.m);
	
	return 0;
}

void cv_init(struct condvar* cv)
{
  InitializeCriticalSection(&cv->wakeup_lock);
  cv->first_wakeup = NULL;
  cv->last_wakeup = NULL;
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
  w->event = CreateEvent(NULL, //security
                         FALSE, //Auto Reset
                         FALSE, //Initial state = unsignalled
                         NULL); //Name
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

void cv_wakeup_delete(struct thread_wakeup* w)
{
  CloseHandle(w->event);
}

void cv_wait(struct condvar* cv, CRITICAL_SECTION* cs)
{
  struct thread_wakeup w;
  cv_wakeup_add(cv, &w);
  LeaveCriticalSection(cs);
  WaitForSingleObject(w.event, INFINITE);
  cv_wakeup_delete(&w);
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

