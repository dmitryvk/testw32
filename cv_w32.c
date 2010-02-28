#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct condition_variable {
	HANDLE sem;
	HANDLE woke_sem;
	int waiters;
} condition_variable;

void cv_init(struct condition_variable* v);
void cv_wait(struct condition_variable* v, CRITICAL_SECTION* cs);
void cv_signal(struct condition_variable* v);
void cv_broadcast(struct condition_variable* v);

void cv_init(struct condition_variable* v)
{
	// printf("cv_init\n");
	v->sem = CreateSemaphore(NULL, 0, 1000, NULL);
	v->woke_sem = CreateSemaphore(NULL, 0, 1000, NULL);
	v->waiters = 0;
}

void cv_wait(struct condition_variable* v, CRITICAL_SECTION* cs)
{
	printf("cv_wait in %d\n", (int)GetCurrentThreadId());
	v->waiters++;
	LeaveCriticalSection(cs);
	WaitForSingleObject(v->sem, INFINITE);
	printf("cv_wait returned from wait in %d\n", (int)GetCurrentThreadId());
	ReleaseSemaphore(v->woke_sem, 1, NULL);
	EnterCriticalSection(cs);
	v->waiters--;
	printf("cv_wait returned in %d\n", (int)GetCurrentThreadId());
}

void cv_signal(struct condition_variable* v)
{
	printf("cv_signal in %d\n", (int)GetCurrentThreadId());
	if (v->waiters > 0) {
		v->waiters--;
		ReleaseSemaphore(v->sem, 1, NULL);
		WaitForSingleObject(v->woke_sem, INFINITE);
	}
	printf("cv_signal returned in %d\n", (int)GetCurrentThreadId());
}

void cv_broadcast(struct condition_variable* v)
{
	int waiters = v->waiters, i;
	printf("cv_broadcast in %d\n", (int)GetCurrentThreadId());
	ReleaseSemaphore(v->sem, v->waiters, NULL);
	for (i = 0; i < waiters; ++i)
		WaitForSingleObject(v->woke_sem, INFINITE);
	printf("cv_broadcast returned in %d\n", (int)GetCurrentThreadId());
}

typedef enum State { Initial, ServerRequest, ClientResponse } State;

typedef struct ThreadData {
	CRITICAL_SECTION cs;
	State state;
	int server_started;
	int arg;
	condition_variable cv;
} ThreadData;

DWORD WINAPI server_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int i;
	EnterCriticalSection(&data->cs);
	printf("Server started in %d\n", (int)GetCurrentThreadId());
	for (i = 0; i < 100; ++i) {
		data->state = ServerRequest;
		data->arg = i;
		printf("Server send %d\n", data->arg);
		cv_broadcast(&data->cv);
		while (data->state != ClientResponse)
			cv_wait(&data->cv, &data->cs);
		printf("Server received %d\n", data->arg);
	}
	data->state = ServerRequest;
	data->arg = -1;
	printf("Sending exit to client\n");
	cv_broadcast(&data->cv);
	LeaveCriticalSection(&data->cs);
	return 0;
}

DWORD WINAPI client_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int looping = 1;
	EnterCriticalSection(&data->cs);
	printf("Client started in %d\n", (int)GetCurrentThreadId());
	while (looping) {
		while (data->state != ServerRequest)
			cv_wait(&data->cv, &data->cs);
		printf("Client received: %d\n", data->arg);
		if (data->arg == -1) {
			printf("Client exiting\n");
			looping = 0;
		} else {
			data->arg = data->arg * data->arg;
			data->state = ClientResponse;
			printf("Client sent %d\n", data->arg);
			cv_broadcast(&data->cv);
		}
	}
	LeaveCriticalSection(&data->cs);
	return 0;
}

int main(int argc, char *argv[])
{
	ThreadData d;
	HANDLE hserver, hclient;
	InitializeCriticalSection(&d.cs);
	d.state = Initial;
	cv_init(&d.cv);
	
	hserver = CreateThread(NULL, 0, server_thread, (void*)&d, 0, NULL);
	hclient = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	
	WaitForSingleObject(hserver, INFINITE);
	WaitForSingleObject(hclient, INFINITE);
	
	return 0;
}
