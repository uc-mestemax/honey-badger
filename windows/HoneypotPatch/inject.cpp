#include "inject.h"
#include <stdio.h>
#include <vector>
#include <Psapi.h>
#include <tlhelp32.h>
#include "harness_com.h"
#include "ntoskrnl_hooks.h"

#define SepCreateTokenExOffset 0x51D

HMODULE GetCurrentModuleBaseAddress() {
	return GetModuleHandle(NULL);
}

void memcpy_backward(void* dest, const void* src, size_t n) {
	const unsigned char* src_end = (const unsigned char*)src + n;
	unsigned char* dest_end = (unsigned char*)dest + n;

	while (n--) {
		*(--dest_end) = *(--src_end);
	}
}

void InstallJMPHook(uint64_t jmp, void *kpAt) {
	//printf("JMP %p %p\n", jmp, kpAt);
	char buff[] = { 0x48, 0xB9, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,  0x00, 0x00, 0xFF, 0xE1};
	memcpy(&buff[2], &jmp, sizeof(jmp));
	//printf("%p\n", *(uint64_t*)&buff[2]);
	//rw_kernel(buff, kpAt, sizeof(buff));
}

bool InjectNTOSKRNL() {

	printf("[*] Beginning Injection\n\n");

	HANDLE hCurrentProcess = GetCurrentProcess();
	MODULEINFO minfo;
	memset(&minfo, 0, sizeof(minfo));
	if (!GetModuleInformation(hCurrentProcess, NULL, &minfo, sizeof(minfo))) {
		printf("[*] Failed to get module information\n");
		return false;
	}

	HMODULE hHoneyPatch = GetCurrentModuleBaseAddress();
	printf("[*] HoneyPatch base: %p\n", hHoneyPatch);
	printf("[*] HoneyPatch size: %d\n", minfo.SizeOfImage);

	HMODULE hNTOSKRNL = LoadLibrary(L"ntoskrnl.exe");
	uint64_t kpNTOSKRNL = (uint64_t)GetImageBase((char*)"\\SystemRoot\\system32\\ntoskrnl.exe");
	if (!hNTOSKRNL || !kpNTOSKRNL) {
		printf("[*] Failed to load NTOSKRNL %p %p\n", hNTOSKRNL, kpNTOSKRNL);
		return false;
	}

	printf("[*] NTOSKRNL base address: %p\n", kpNTOSKRNL);

	printf("\n[*] Initializing indirects for hooks\n\n");
	InitializeIndirects();


	mapped_phys_mem *pMappedCodeRegion = AllocExecutable(minfo.SizeOfImage, false);

	if (!pMappedCodeRegion->pKVA) {
		printf("[*] Failed to map code region\n");
		return false;
	}

	printf("[*] New code region resides at: %p-%p\n", pMappedCodeRegion->pKVA, (char*)pMappedCodeRegion->pKVA+minfo.SizeOfImage);

	printf("[*] Writing to new code region\n");
	rw_kernel(hHoneyPatch, pMappedCodeRegion->pKVA, minfo.SizeOfImage);

	if (!GetModuleInformation(hCurrentProcess, hNTOSKRNL, &minfo, sizeof(minfo))) {
		printf("[*] Failed to get module information for NTOSKRNL\n");
		return false;
	}

	printf("[*] Preparing to install hooks, setting %p-%p to r+w\n", kpNTOSKRNL, kpNTOSKRNL+minfo.SizeOfImage);
	MMSecureMem((void*)kpNTOSKRNL, minfo.SizeOfImage, true);

	//uint64_t SepCreateTokenEx = (uint64_t)GetProcAddress(hNTOSKRNL, "SepCreateTokenEx");
	uint64_t SepCreateTokenEx = (uint64_t)hNTOSKRNL + 0x2FB674;
	uint64_t ObfDereferenceObjectWithTag = (uint64_t)GetProcAddress(hNTOSKRNL, "ObfDereferenceObjectWithTag");
	//uint64_t KiPageFault = (uint64_t)GetProcAddress(hNTOSKRNL, "KiPageFault");
	uint64_t KiPageFault = (uint64_t)hNTOSKRNL + 0x40B480;

	if (!SepCreateTokenEx || !ObfDereferenceObjectWithTag || !KiPageFault) {
		printf("[*] Failed to resolve function addresses for hooks\n");
		return false;
	}
	//Add required offsets
	SepCreateTokenEx + SepCreateTokenExOffset;

	printf("[*] Installing hooks for:\n");
	printf("	SepCreateTokenEx %p redirected to -> %p\n", (uint64_t)SepCreateTokenExHook - (uint64_t)hHoneyPatch + (uint64_t)pMappedCodeRegion->pKVA, (void*)((uint64_t)SepCreateTokenEx - (uint64_t)hNTOSKRNL + kpNTOSKRNL));
	InstallJMPHook((uint64_t)SepCreateTokenExHook - (uint64_t)hHoneyPatch + (uint64_t)pMappedCodeRegion->pKVA, (void*)((uint64_t)SepCreateTokenEx-(uint64_t)hNTOSKRNL + kpNTOSKRNL));

	printf("	ObfDereferenceObjectWithTag %p redirected to -> %p\n", ObfDereferenceObjectWithTag, ObfDereferenceObjectWithTagHook);
	printf("	KiPageFault %p redirected to -> %p\n", KiPageFault, KiPageFaultHook);



	free(pMappedCodeRegion);
	printf("\n[*] Finished Injecting\n");

	return true;

fail:
	printf("[*] Failed to install hook\n");
	free(pMappedCodeRegion);
	return false;
}