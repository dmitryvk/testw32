#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>

typedef enum {
	ProcessLdtInformation = 10
} ProcessInformationClass;

typedef LONG (WINAPI * NtQueryInformationProcess_t)
(	HANDLE ProcessHandle,
	ProcessInformationClass  informationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
);

extern NtQueryInformationProcess_t NtQueryInformationProcess;

typedef LONG (WINAPI * NtSetInformationProcess_t)
(	HANDLE ProcessHandle,
	ProcessInformationClass  informationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
);

extern NtSetInformationProcess_t NtSetInformationProcess;

typedef LONG (WINAPI * NtSetLdtEntries_t) (
	ULONG Selector1,
	LDT_ENTRY Entry1,
	ULONG Selector2,
	LDT_ENTRY Entry2
);

extern NtSetLdtEntries_t NtSetLdtEntries;

typedef struct {
	ULONG Start;
	ULONG Length;
	LDT_ENTRY LdtEntries[1];
} PROCESS_LDT_INFORMATION;

extern void init_nt();
