#include "seg_gs.h"
#include "nt.h"

int GetLdtCount()
{
	PROCESS_LDT_INFORMATION information;
	information.Start = 0;
	information.Length = 8;
	NtQueryInformationProcess(GetCurrentProcess(), ProcessLdtInformation, &information, sizeof(information), NULL);
	return information.Length / sizeof(LDT_ENTRY);
}

int AllocateLdtSegment(void * base, int size)
{
	int index = GetLdtCount();
	
	LDT_ENTRY entry, blank;
	DWORD baseb = (DWORD)base;
	
	if (index == 0)
		index = 1;
	
	index = (index << 3) | 7;
	
	memset(&blank, 0, sizeof(&blank));
	
	
	//fprintf(stderr, "Limit: %x, low: %x, high: %x\n", size, (int)(size & 0xFFFF), (int)((size>>16) & 0xFF));
	
	entry.LimitLow = size & 0xFFFF;
	entry.HighWord.Bits.LimitHi = (size >> 16) & 0xFF;
	
	entry.BaseLow = baseb & 0xFFFF;
	entry.HighWord.Bytes.BaseMid = (baseb >> 16) & 0xFF;
	entry.HighWord.Bytes.BaseHi = (baseb >> 24) & 0xFF;
	
	entry.HighWord.Bits.Granularity = 0; //Limit is in bytes, not 4k blocks
	
	entry.HighWord.Bits.Type = 16 | 3; // Code or data segment
	
	entry.HighWord.Bits.Dpl = 3; // Access from Ring 3
	
	entry.HighWord.Bits.Pres = 1; // Segment is present
	
	entry.HighWord.Bits.Sys = 0;
	entry.HighWord.Bits.Reserved_0 = 0;
	
	entry.HighWord.Bits.Default_Big = 1; // 32bit segment
	
	if (NtSetLdtEntries(index, entry, 0, blank) == 0)
		return index;
	else
		return 0;
}

// void SaveGsIntoTls(int selector)
// {
	// asm("movl %0, %%fs:0x14"::"r"(selector));
// }

// void ReloadGs()
// {
	// int selector;
	// asm("movl %%fs:0x14, %0":"=r"(selector):);
	// asm("movl %0, %%fs:0x14"::"r"(selector));
// }

// int ReadGsRelative(int offset)
// {
	// int result;
	// asm("movl %%gs:(%0), %1": "=r"(result):"r"(offset));
	// return result;
// }

// void WriteGsRelative(int offset, int value)
// {
	// asm("mov %0, %%gs:(%1)":: "r"(value), "r"(offset));
// }
