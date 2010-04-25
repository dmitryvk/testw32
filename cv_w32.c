#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "pthreads_win32.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum State { Initial, ServerRequest, ClientResponse } State;

typedef struct ThreadData {
	pthread_mutex_t m;
	pthread_cond_t cv;
	State state;
	int server_started;
	int arg;
} ThreadData;

void* server_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int i;
  pthread_mutex_lock(&data->m);
	printf("Server started in %d\n", (int)pthread_self());
	for (i = 0; i < 100; ++i) {
		data->state = ServerRequest;
		data->arg = i;
		printf("%d: Server send %d\n", (int)pthread_self(), data->arg);
		pthread_cond_broadcast(&data->cv);
		while (data->state != ClientResponse)
      pthread_cond_wait(&data->cv, &data->m);
		printf("%d: Server received %d\n", (int)pthread_self(), data->arg);
	}
	data->state = ServerRequest;
	data->arg = -1;
	printf("Sending exit to client\n");
	pthread_cond_broadcast(&data->cv);
	pthread_mutex_unlock(&data->m);
	return NULL;
}

void* client_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int looping = 1;
	pthread_mutex_lock(&data->m);
	printf("Client started in %d\n", (int)pthread_self());
	while (looping) {
		while (data->state != ServerRequest)
			pthread_cond_wait(&data->cv, &data->m);
		printf("%d: Client received: %d\n", (int)pthread_self(), data->arg);
		if (data->arg == -1) {
			printf("%d: Client exiting\n", (int)pthread_self());
			looping = 0;
		} else {
			data->arg = data->arg * data->arg;
			data->state = ClientResponse;
			printf("%d: Client sent %d\n", (int)pthread_self(), data->arg);
			pthread_cond_broadcast(&data->cv);
		}
	}
	pthread_mutex_unlock(&data->m);
	return NULL;
}

void pthread_np_safepoint()
{
  fprintf(stderr, "%d: safepoint\n", (int)pthread_self());
}

int main(int argc, char *argv[])
{
	ThreadData d;
	pthread_t hserver, hclient, h1, h2, h3;
  pthreads_win32_init();
  pthread_mutex_init(&d.m, NULL);
  pthread_cond_init(&d.cv, NULL);
	d.state = Initial;
	
	pthread_create(&hserver, NULL, server_thread, (void*)&d);
	pthread_create(&hclient, NULL, client_thread, (void*)&d);
	pthread_create(&h1, NULL, client_thread, (void*)&d);
	pthread_create(&h2, NULL, client_thread, (void*)&d);
	pthread_create(&h3, NULL, client_thread, (void*)&d);
	
	pthread_join(hserver, NULL);
  fprintf(stderr, "ok1\n");
	pthread_join(hclient, NULL);
  fprintf(stderr, "ok2\n");
  //EnterCriticalSection(&d.m);
  //cv_bcast(&d.cv);
  //LeaveCriticalSection(&d.m);
	pthread_join(h1, NULL);
  fprintf(stderr, "ok3\n");
	pthread_join(h2, NULL);
  fprintf(stderr, "ok4\n");
	pthread_join(h3, NULL);
  fprintf(stderr, "ok5\n");
  
  pthread_cond_destroy(&d.cv);
  pthread_mutex_destroy(&d.m);
	
	return 0;
}
