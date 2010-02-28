#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>

#include "nt.h"
#include "seg_gs.h"

#include <stdio.h>
#include <stdlib.h>

void LdtOutput()
{
	HANDLE hProcess = GetCurrentProcess();
    PROCESS_LDT_INFORMATION LdtInfo,*LdtInfoArray;
    ULONG LdtInfoSize,LdtCount;
    LDT_ENTRY MyDescriptor;
    DWORD dwBase,dwLimit;
    UINT i;

    LdtInfo.Start = 0;
    LdtInfo.Length = 8;
    NtQueryInformationProcess(hProcess,ProcessLdtInformation,&LdtInfo,sizeof(LdtInfo),NULL);

    LdtInfoSize = 8 + LdtInfo.Length;
    LdtCount = LdtInfo.Length/sizeof(LDT_ENTRY);

    if(LdtCount==0)
    {
        printf("LDT is empty\n");
        return;
    }

    LdtInfoArray = (PROCESS_LDT_INFORMATION*)malloc(LdtInfoSize);
    LdtInfoArray->Start = 0;
    LdtInfoArray->Length = LdtCount*sizeof(LDT_ENTRY);
    NtQueryInformationProcess(hProcess,ProcessLdtInformation,LdtInfoArray,
                              LdtInfoSize,NULL);

    for(i=0;i<LdtCount;i++)
    {
        printf("LDT Entry %d\n",i);
        MyDescriptor = LdtInfoArray->LdtEntries[i];

        dwBase = MyDescriptor.BaseLow;
        dwBase |= MyDescriptor.HighWord.Bits.BaseMid<<16;
        dwBase |= MyDescriptor.HighWord.Bits.BaseHi<<24;
        dwLimit = MyDescriptor.LimitLow;
        dwLimit |= MyDescriptor.HighWord.Bits.LimitHi<<16;
        if(MyDescriptor.HighWord.Bits.Granularity)
            dwLimit = ((dwLimit+1)*0x1000)-1;
        printf("\tBase=%08X\n",dwBase);
        printf("\tLimit=%08X\n",dwLimit);
        printf("\tAVL=%d\n",MyDescriptor.HighWord.Bits.Sys);
        printf("\tD/B=%d\n",MyDescriptor.HighWord.Bits.Default_Big);
        printf("\tDPL=%d\n",MyDescriptor.HighWord.Bits.Dpl);
        printf("\tG=%d\n",MyDescriptor.HighWord.Bits.Granularity);
        printf("\tP=%d\n",MyDescriptor.HighWord.Bits.Pres);
        printf("\tS=%d\n",(MyDescriptor.HighWord.Bits.Type&16)>>4);
        printf("\tType=%d\n",MyDescriptor.HighWord.Bits.Type&15);
    }
    free(LdtInfoArray);
}

DWORD WINAPI thread_fn(void * arg)
{
	int selector = (int)arg, r1, r2, i;
	CRITICAL_SECTION cs;
	InitializeCriticalSection(&cs);
	printf("selector = 0x%x\n", selector);
	printf("saving into tls\n");
	SaveGsIntoTls(selector);
	printf("loading gs\n");
	ReloadGs();
	printf("reading (and loading gs)\n");
	ReloadGs();
	asm("movl %%esi, %0":"=r"(i):);
	printf("esi = 0x%x\n", i);
	ReloadGs();
	while (1) {
		EnterCriticalSection(&cs);
		ReloadGs();
		for (i = 0; i < 10000000; ++i) {
			ReloadGs();
			r1 = ReadGsRelative(0);
			ReloadGs();
			r2 = ReadGsRelative(4);
		}
		LeaveCriticalSection(&cs);
		ReloadGs();
		printf("Read from thread (selector = 0x%x): 0x%x 0x%x\n", selector, r1, r2);
		ReloadGs();
	}
	WriteGsRelative(8, 123);
	return 0;
}

int main (int argc, char *argv[])
{
    void * region;
    int selector;
	volatile int tmp1, tmp2;
    HANDLE t1;
	init_nt();

for (tmp1 = 0; tmp1 < 10; ++tmp1) {
	
#define REGION_SIZE (512 * 1024)
	region = VirtualAlloc(NULL, REGION_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    selector = AllocateLdtSegment(region, REGION_SIZE);

	*((int*)region) = (int)selector;
	*((int*)(region+4)) = (int)region;
	
	t1 = CreateThread(NULL, 0, thread_fn, (void*)selector, 0, NULL);

	}
	
	WaitForSingleObject(t1, INFINITE);

	ReloadGs();
	tmp1 = ReadGsRelative(8);
	printf("Thread has written: %d\n", tmp1);
	
	return 0;
}
