#include "nt.h"

NtQueryInformationProcess_t NtQueryInformationProcess;
NtSetInformationProcess_t NtSetInformationProcess;
NtSetLdtEntries_t NtSetLdtEntries;

void init_nt()
{
	HANDLE ntdll = LoadLibraryA("ntdll.dll");
	NtQueryInformationProcess = (NtQueryInformationProcess_t)GetProcAddress(ntdll, "NtQueryInformationProcess");
	NtSetInformationProcess = (NtSetInformationProcess_t)GetProcAddress(ntdll, "NtSetInformationProcess");
	NtSetLdtEntries = (NtSetLdtEntries_t)GetProcAddress(ntdll, "NtSetLdtEntries");
}
