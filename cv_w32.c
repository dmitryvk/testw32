#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "pthreads_win32.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum State { Initial, ServerRequest, ClientResponse } State;

typedef enum ThreadState { Running, Suspended } ThreadState;

typedef struct thread_information {
  pthread_t thread;
  ThreadState state;
  pthread_mutex_t state_lock;
  pthread_cond_t state_cond;
  
  struct thread_information * next;
  unsigned char suspended_by_suspend;
  unsigned char suspending;
  unsigned char gc_master;
} thread_information;

pthread_mutex_t threads_lock;
thread_information * threads;
unsigned char suspend_pending = 0;

DWORD tls_ti;

void register_self()
{
  pthread_mutex_lock(&threads_lock);
  {
    thread_information * information = (thread_information*)malloc(sizeof(thread_information));
    information->thread = pthread_self();
    information->state = Running;
    information->gc_master = 0;
    information->suspending = 0;
    pthread_mutex_init(&information->state_lock, NULL);
    pthread_cond_init(&information->state_cond, NULL);
    information->next = threads;
    threads = information;
    TlsSetValue(tls_ti, information);
  }
  pthread_mutex_unlock(&threads_lock);
}

thread_information* get_current_thread_information()
{
  return (thread_information*)TlsGetValue(tls_ti);
}

ThreadState get_thread_state(thread_information* inf)
{
  if (inf == NULL) {
    inf = get_current_thread_information();
  }
  if (inf == NULL) return Suspended;
  return inf->state;
}

void set_thread_state(thread_information* inf, ThreadState state)
{
  if (inf == NULL) {
    inf = get_current_thread_information();
  }
  if (inf == NULL) return;
  //fprintf(stderr, " 0x%p,sts:1\n", inf);
  pthread_mutex_lock(&inf->state_lock);
  //fprintf(stderr, " 0x%p,sts:2\n", inf);
  inf->state = state;
  pthread_cond_broadcast(&inf->state_cond);
  //fprintf(stderr, " 0x%p,sts:3\n", inf);
  pthread_mutex_unlock(&inf->state_lock);
  //fprintf(stderr, " 0x%p,sts:4\n", inf);
}

void wait_for_thread_state(thread_information* inf, ThreadState state)
{
  if (inf == NULL) {
    inf = get_current_thread_information();
  }
  if (inf == NULL) return;
  pthread_mutex_lock(&inf->state_lock);
  while (inf->state != state)
    pthread_cond_wait(&inf->state_cond, &inf->state_lock);
  pthread_mutex_unlock(&inf->state_lock);
}

typedef struct ThreadData {
	pthread_mutex_t m;
	pthread_cond_t cv;
	State state;
	int server_started;
	int arg;
} ThreadData;

void pthread_np_safepoint()
{
  thread_information* inf;
  //fprintf(stderr, "%d: safepoint\n", (int)pthread_self());
  if (suspend_pending && !(inf = get_current_thread_information())->suspending && !inf->gc_master) {
    inf->suspending = 1;
    set_thread_state(NULL, Suspended);
    wait_for_thread_state(NULL, Running);
    inf->suspending = 0;
  }
}

void suspend_thread(thread_information* thread)
{
  pthread_t p_thread = thread->thread;
  pthread_np_suspend(p_thread);
  if (get_thread_state(thread) == Suspended) {
    thread->suspended_by_suspend = 0;
    pthread_np_resume(p_thread);
  } else if (pthread_np_interruptible(p_thread)) {
    thread->suspended_by_suspend = 1;
    set_thread_state(thread, Suspended);
  } else {
    thread->suspended_by_suspend = 0;
    pthread_np_request_interruption(p_thread);
    pthread_np_resume(p_thread);
    wait_for_thread_state(thread, Suspended);
  }
}

void resume_thread(thread_information* thread)
{
  pthread_t p_thread = thread->thread;
  set_thread_state(thread, Running);
  if (thread->suspended_by_suspend) {
    pthread_np_resume(p_thread);
  }
}

void suspend_threads()
{
  pthread_mutex_lock(&threads_lock);
  get_current_thread_information()->gc_master = 1;
  suspend_pending = 1;
  {
    pthread_t self = pthread_self();
    thread_information * ti;
    for (ti = threads; ti; ti = ti->next) {
      if (!pthread_equal(self, ti->thread))
        suspend_thread(ti);
    }
  }
  suspend_pending = 0;
  get_current_thread_information()->gc_master = 0;
  pthread_mutex_unlock(&threads_lock);
}

void resume_threads()
{
  pthread_mutex_lock(&threads_lock);
  {
    pthread_t self = pthread_self();
    thread_information * ti;
    for (ti = threads; ti; ti = ti->next) {
      if (!pthread_equal(self, ti->thread))
        resume_thread(ti);
    }
  }
  pthread_mutex_unlock(&threads_lock);
}

void gc()
{
  fprintf(stderr, "I'm gcing! 1. Suspend\n");
  suspend_threads();
  fprintf(stderr, "Suspended. GCing\n");
  fprintf(stderr, "2. Resume\n");
  resume_threads();
}

void* server_thread(void * arg)
{
	ThreadData* data = (ThreadData*)arg;
	int i;
  register_self();
  pthread_mutex_lock(&data->m);
	printf("Server started in %d\n", (int)pthread_self());
  while (1) {
    for (i = 0; i < 1000; ++i) {
      data->state = ServerRequest;
      data->arg = i;
      //printf("%d: Server send %d\n", (int)pthread_self(), data->arg);
      pthread_cond_broadcast(&data->cv);
      while (data->state != ClientResponse)
        pthread_cond_wait(&data->cv, &data->m);
      //printf("%d: Server received %d\n", (int)pthread_self(), data->arg);
    }
    pthread_mutex_unlock(&data->m);
    gc();
    pthread_mutex_lock(&data->m);
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
	int looping = 1, i = 0;
  register_self();
	pthread_mutex_lock(&data->m);
	printf("Client started in %d\n", (int)pthread_self());
	while (looping) {
		while (data->state != ServerRequest)
			pthread_cond_wait(&data->cv, &data->m);
		//printf("%d: Client received: %d\n", (int)pthread_self(), data->arg);
		if (data->arg == -1) {
			printf("%d: Client exiting\n", (int)pthread_self());
			looping = 0;
		} else {
			data->arg = data->arg * data->arg;
			data->state = ClientResponse;
			//printf("%d: Client sent %d\n", (int)pthread_self(), data->arg);
			pthread_cond_broadcast(&data->cv);
		}
    i = (i + 1) % 1000;
    if (i == 0)
      fprintf(stderr, "  Client 0x%p is working\n", pthread_self());
	}
	pthread_mutex_unlock(&data->m);
	return NULL;
}

int main(int argc, char *argv[])
{
	ThreadData d;
	pthread_t hserver, hclient, h1, h2, h3;
  tls_ti = TlsAlloc();
  pthreads_win32_init();
  threads = NULL;
  pthread_mutex_init(&threads_lock, NULL);
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
