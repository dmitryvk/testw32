#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>

// #include "nt.h"
// #include "seg_gs.h"

#include <stdio.h>
#include <stdlib.h>

DWORD WINAPI thread_fn(void * arg)
{
	int z = 0;
	fprintf(stderr, "Entering sleep\n");
	Sleep(1000);
	fprintf(stderr, "Returned from sleep\n");
	while (1) {
		Sleep(10);
		printf("doing nothing\n");
		for (z = 0; z < 1000000000; ++z);
	}
	return 0;
}

typedef void (*planted_function_t)(void * arg);

extern void planted_trampoline();

// planting can only occur in a user mode; do not attempt to plant function call when thread is in kernel
void plant_call(HANDLE thread, planted_function_t fn, void * arg)
{
	CONTEXT context;
	if (SuspendThread(thread) == -1)
	{
		printf("Unable to suspend thread 0x%x\n", (int)thread);
		return;
	}
	context.ContextFlags = CONTEXT_FULL;
	if (GetThreadContext(thread, &context) == 0)
	{
		printf("Unable to get thread context for thread 0x%x\n", (int) thread);
		ResumeThread(thread);
		return;
	}
	
	{
		//planting
		//calling convention is as follow:
		// PUSH unsaved registers
		// PUSH last arg
		// ...
		// PUSH first arg
		// PUSH return address           }
		// SET %EIP to first instruction } CALL does this
		// POP unsaved registers
		
		// we should save registers (TODO!)
		
		// we have no args, so don't push
		
		// pushing return address (the current EIP, which points to the next instruction)
		context.Esp -= 4;
		*((int*)((void*)context.Esp - 0)) = context.Eip;
		context.Esp -= 4;
		*((int*)((void*)context.Esp - 0)) = (int)arg;
		context.Esp -= 4;
		*((int*)((void*)context.Esp - 0)) = (int)(void*)fn;
		// setting %EIP
		context.Eip = (int)(void*)planted_trampoline;
	}
	if (SetThreadContext(thread, &context) == 0)
	{
		printf("Unable to get thread context for thread 0x%x\n", (int) thread);
		ResumeThread(thread);
		return;
	}
	
	if (ResumeThread(thread) == -1)
	{
		printf("Unable to resume thread 0x%x\n", (int)thread);
		return;
	}
	printf("Function planted to thread 0x%x\n", (int)thread);
}

void planted_fn(void* arg)
{
	int current_thread = (int)GetCurrentThreadId();
	printf("Called from thread = 0x%x with arg = \"%s\"\n", current_thread, (char*)arg);
}

int main(int argc, char * argv[])
{
	DWORD thread_id;
	HANDLE thread = CreateThread(NULL, 0, thread_fn, NULL, 0, &thread_id);

	printf("Will plant into thread = 0x%x\n", (int)thread_id);

	Sleep(100);

	fprintf(stderr, "planting\n");
	
	plant_call(thread, planted_fn, (void*)"testing planting");
	
	WaitForSingleObject(thread, INFINITE);
	
	return 0;
}
