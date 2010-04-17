#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "cv.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum State { Initial, ServerRequest, ClientResponse } State;

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

VOID CALLBACK test_apc(ULONG_PTR param)
{
  fprintf(stderr, "APC!!!!! %s\n", (unsigned char*)param);
}

DWORD event_tls;

HANDLE get_fn()
{
  HANDLE h = TlsGetValue(event_tls);
  if (!h) {
    h = (HANDLE)CreateEvent(NULL, FALSE, FALSE, NULL);
    TlsSetValue(event_tls, h);
  }
  return h;
}

void return_fn(HANDLE h)
{
}

int main(int argc, char *argv[])
{
	ThreadData d;
	HANDLE hserver, hclient, h1, h2, h3;
  InitializeCriticalSection(&d.m);
  if (0)
    cv_init(&d.cv, TRUE, NULL, NULL);
  else {
    event_tls = TlsAlloc();
    cv_init(&d.cv, TRUE, get_fn, return_fn);
  }
	d.state = Initial;
	
	hserver = CreateThread(NULL, 0, server_thread, (void*)&d, 0, NULL);
	hclient = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h1 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h2 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h3 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	
  SuspendThread(h2);
  QueueUserAPC(test_apc, h2, (ULONG_PTR)"qwe1");
  QueueUserAPC(test_apc, h2, (ULONG_PTR)"qwe2");
  ResumeThread(h2);
  
	WaitForSingleObject(hserver, INFINITE);
  fprintf(stderr, "ok1\n");
	WaitForSingleObject(hclient, INFINITE);
  fprintf(stderr, "ok2\n");
  //EnterCriticalSection(&d.m);
  //cv_bcast(&d.cv);
  //LeaveCriticalSection(&d.m);
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
