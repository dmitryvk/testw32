#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

typedef enum State { Initial, ServerRequest, ClientResponse } State;

typedef struct ThreadData {
	pthread_mutex_t m;
	pthread_cond_t cv;
	State state;
	int server_started;
	int arg;
} ThreadData;

DWORD WINAPI server_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int i;
	Sleep(100);
	pthread_mutex_lock(&data->m);
	printf("Server started in %d\n", (int)GetCurrentThreadId());
	for (i = 0; i < 100; ++i) {
		data->state = ServerRequest;
		data->arg = i;
		printf("%d: Server send %d\n", (int)GetCurrentThreadId(), data->arg);
		pthread_cond_broadcast(&data->cv);
		while (data->state != ClientResponse)
			pthread_cond_wait(&data->cv, &data->m);
		printf("%d: Server received %d\n", (int)GetCurrentThreadId(), data->arg);
	}
	data->state = ServerRequest;
	data->arg = -1;
	printf("Sending exit to client\n");
	pthread_cond_broadcast(&data->cv);
	pthread_mutex_unlock(&data->m);
	return 0;
}

DWORD WINAPI client_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int looping = 1;
	pthread_mutex_lock(&data->m);
	printf("Client started in %d\n", (int)GetCurrentThreadId());
	while (looping) {
		while (data->state != ServerRequest)
			pthread_cond_wait(&data->cv, &data->m);
		printf("%d: Client received: %d\n", (int)GetCurrentThreadId(), data->arg);
		if (data->arg == -1) {
			printf("%d: Client exiting\n", (int)GetCurrentThreadId());
			looping = 0;
		} else {
			data->arg = data->arg * data->arg;
			data->state = ClientResponse;
			printf("%d: Client sent %d\n", (int)GetCurrentThreadId(), data->arg);
			pthread_cond_broadcast(&data->cv);
		}
	}
	pthread_mutex_unlock(&data->m);
	return 0;
}

int main(int argc, char *argv[])
{
	ThreadData d;
	HANDLE hserver, hclient, h1, h2;
	pthread_mutex_init(&d.m, NULL);
	pthread_cond_init(&d.cv, NULL);
	d.state = Initial;
	
	hserver = CreateThread(NULL, 0, server_thread, (void*)&d, 0, NULL);
	hclient = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h1 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	h2 = CreateThread(NULL, 0, client_thread, (void*)&d, 0, NULL);
	
	WaitForSingleObject(hserver, INFINITE);
	WaitForSingleObject(hclient, INFINITE);
	WaitForSingleObject(h1, INFINITE);
	WaitForSingleObject(h2, INFINITE);
	
	return 0;
}
