#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>

#ifndef SEG_GS_INCLUDED
#define SEG_GS_INCLUDED

extern int AllocateLdtSegment(void * base, int size);

// extern void SaveGsIntoTls(int selector);

// extern void ReloadGs();

// extern int ReadGsRelative(int offset);

// extern void WriteGsRelative(int offset, int value);


static inline void SaveGsIntoTls(int selector)
{
	asm("movl %0, %%fs:0x14"::"r"(selector));
}

static inline void ReloadGs()
{
	int selector;
	asm("movl %%fs:0x14, %0":"=r"(selector):);
	asm("movl %0, %%gs"::"r"(selector));
}

static inline int ReadGsRelative(int offset)
{
	int result;
	asm("movl %%gs:(%0), %1": "=r"(result):"r"(offset));
	return result;
}

static inline void WriteGsRelative(int offset, int value)
{
	asm("mov %0, %%gs:(%1)":: "r"(value), "r"(offset));
}

#endif
