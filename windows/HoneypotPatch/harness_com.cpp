#include "harness_com.h"
#include <stdio.h>

HANDLE ghHarnessDrv = NULL;

void InitMemAction(MemActionHdr_s* pHdr, size_t size, MemActionType type) {
	memset(pHdr, 0, size);
	pHdr->type = type;
}

bool send_memaction(MemActionHdr_s* pAction) {
	DWORD numRetBytes = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_MEM_ACTION, &pAction, sizeof(pAction), NULL, 0, &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return false;
	}
	return true;
}

mapped_phys_mem* AllocContiguous(size_t size, uint64_t highestAddress, bool mapUVA) {
	if (size & 0xFFF != 0) {
		printf("Contiguous memory allocation must be page aligned! Passed a size of: %p\n", size);
		return NULL;
	}
	MemAction_AllocContiguous maction;
	InitMemAction(&maction.h, sizeof(MemAction_AllocContiguous), MEMACTION_ALLOC_CONTIGUOUS);
	maction.highestAddress = highestAddress;
	maction.size = size;
	maction.mapUVA = mapUVA;

	mapped_phys_mem* mappedInfo = (mapped_phys_mem*)malloc(sizeof(mapped_phys_mem));
	memset(mappedInfo, 0, sizeof(mapped_phys_mem));
	if (!send_memaction(&maction.h)) {
		free(mappedInfo);
		return NULL;
	}
	mappedInfo->physMem = maction.h.result.physAddress;
	mappedInfo->pKVA = maction.h.result.kVA;
	mappedInfo->pUVA = maction.h.result.uVA;
	return mappedInfo;
}

mapped_phys_mem* AllocExecutable(size_t size, bool mapUVA) {
	MemAction_AllocContiguous maction;
	InitMemAction(&maction.h, sizeof(MemAction_AllocContiguous), MEMACTION_ALLOC_EXECUTABLE);
	maction.size = size;
	maction.mapUVA = mapUVA;

	mapped_phys_mem* mappedInfo = (mapped_phys_mem*)malloc(sizeof(mapped_phys_mem));
	memset(mappedInfo, 0, sizeof(mapped_phys_mem));
	if (!send_memaction(&maction.h)) {
		free(mappedInfo);
		return NULL;
	}
	mappedInfo->physMem = maction.h.result.physAddress;
	mappedInfo->pKVA = maction.h.result.kVA;
	mappedInfo->pUVA = maction.h.result.uVA;
	return mappedInfo;
}

//Size must be page aligned
mapped_phys_mem* AllocAndMapContiguous(size_t size, uint64_t highestAddress) {
	return AllocContiguous(size, highestAddress, true);
}
uint64_t msr_action(uint index, long long newValue, bool write) {
	DWORD numRetBytes = 0;
	msr_action_s action = { index, newValue, write };
	uint64_t oldValue = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_MSR, &action, sizeof(action), &oldValue, sizeof(oldValue), &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return NULL;
	}
	return oldValue;
}

void* userMapPhys(PHYSICAL_ADDRESS address, size_t size) {
	MemAction_MapPhys maction;
	InitMemAction(&maction.h, sizeof(MemAction_MapPhys), MEMACTION_MAP_PHYS);
	maction.phys = address;
	maction.size = size;
	maction.mapUVA = true;
	if (!send_memaction(&maction.h)) {
		return NULL;
	}
	return maction.h.result.uVA;
}

void* kernelMapPhys(PHYSICAL_ADDRESS address, size_t size) {
	MemAction_MapPhys maction;
	InitMemAction(&maction.h, sizeof(MemAction_MapPhys), MEMACTION_MAP_PHYS);
	maction.phys = address;
	maction.size = size;
	maction.mapUVA = false;
	if (!send_memaction(&maction.h)) {
		return NULL;
	}
	return maction.h.result.kVA;
}

void OpenHarnessDrv() {
	ghHarnessDrv = CreateFileW(L"\\\\.\\GLOBALROOT\\Device\\HarnessDrv", GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (ghHarnessDrv == INVALID_HANDLE_VALUE) {
		printf("Failed to open handle %d\n", GetLastError());
		ghHarnessDrv = NULL;
	}
}

PHYSICAL_ADDRESS get_physical_address(void* pVA) {
	MemAction_GetPhys maction;
	InitMemAction(&maction.h, sizeof(MemAction_GetPhys), MEMACTION_GET_PHYSICAL);
	maction.pVA = pVA;
	if (!send_memaction(&maction.h)) {
		return NULL;
	}
	return maction.h.result.physAddress;
}

void* get_virtual_address(PHYSICAL_ADDRESS address) {
	MemAction_GetVA maction;
	InitMemAction(&maction.h, sizeof(MemAction_GetVA), MEMACTION_GET_VIRTUAL);
	maction.phys = address;
	if (!send_memaction(&maction.h)) {
		return NULL;
	}
	return maction.h.result.kVA;
}

void* map_virtual_address(void* pAddress, size_t size) {
	MemAction_MapVA maction;
	InitMemAction(&maction.h, sizeof(MemAction_GetVA), MEMACTION_MAP_VA);
	maction.pVA = pAddress;
	maction.size = size;
	if (!send_memaction(&maction.h)) {
		return NULL;
	}
	return maction.h.result.uVA;
}

bool rw_kernel(void* pSrc, void* pDst, size_t size) {
	MemAction_RW maction;
	InitMemAction(&maction.h, sizeof(MemAction_RW), MEMACTION_RW);
	maction.pDst = pDst;
	maction.pSrc = pSrc;
	maction.size = size;
	if (!send_memaction(&maction.h)) {
		return false;
	}
	return true;
}

bool MMSecureMem(void* pAddress, size_t size, bool rw) {
	MemAction_MMSecureV maction;
	InitMemAction(&maction.h, sizeof(MemAction_MMSecureV), MEMACTION_MMSECUREV);
	maction.va = pAddress;
	maction.size = size;
	maction.rw = rw;
	if (!send_memaction(&maction.h)) {
		return false;
	}
	return true;
}

bool get_GTD(gdtr_s* pGDT) {
	DWORD numRetBytes = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_GET_GTD, NULL, 0, pGDT, sizeof(gdtr_s), &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return false;
	}
	return true;
}

bool get_ITD(idtr_s* pIDTR) {
	DWORD numRetBytes = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_GET_ITD, NULL, 0, pIDTR, sizeof(idtr_s), &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return false;
	}
	return true;
}

void* kvirtual_alloc(void* pVirtual, size_t sz) {
	MemAction_AllocKVA maction;
	InitMemAction(&maction.h, sizeof(MemAction_AllocKVA), MEMACTION_ALLOC_KVIRTUAL);
	maction.size = sz;
	maction.mapUVA = NULL;
	printf("KVirtualAlloc not implimented!");
	exit(-1);

	if (!send_memaction(&maction.h)) {
		return NULL;
	}
	return NULL;
}

void* GetImageBase(char* pImageName) {
	DWORD numRetBytes = 0;
	void* pImageBase = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_GET_IMAGE_BASE, &pImageName, sizeof(char*), &pImageBase, sizeof(pImageBase), &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return NULL;
	}
	return pImageBase;
}

bool unloadHarnessDrv() {
	DWORD numRetBytes = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_IO_UNLOAD, NULL, NULL, NULL, NULL, &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return false;
	}
	ghHarnessDrv = NULL;
	return true;
}

copy_execute_s* SetupCustomExecute(void* pFunction, size_t size, uint numArgs, ...) {
	va_list a_list;
	va_start(a_list, numArgs);

	copy_execute_s* pCE = (copy_execute_s*)malloc(sizeof(copy_execute_s) + sizeof(void*) * size);
	memset(pCE, 0, sizeof(copy_execute_s) + sizeof(void*) * size);
	pCE->pSrc = pFunction;
	pCE->size = size;
	pCE->numArgs = numArgs;
	for (uint i = 0; i < numArgs; i++) {
		pCE->args[i] = va_arg(a_list, void*);
	}
	va_end(a_list);
	return pCE;
}

bool PrepareCopyExecuteASM(copy_execute_s* pCE) {
	DWORD numRetBytes = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_PREPARE_COPY_EXECUTE, &pCE, sizeof(copy_execute_s*), NULL, NULL, &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return false;
	}
	return true;
}

bool RunCopyExecuteASM(copy_execute_s* pCE) {
	DWORD numRetBytes = 0;
	if (!DeviceIoControl(ghHarnessDrv, IOCTL_RUN_COPY_EXECUTE, &pCE, sizeof(copy_execute_s*), NULL, NULL, &numRetBytes, NULL)) {
		printf("Failed to send IOCTL %d\n", GetLastError());
		printf("Failed to send IOCTL %d\n", numRetBytes);
		return false;
	}
	return true;
}

bool CopyExecuteASM(copy_execute_s* pCE) {
	if (!PrepareCopyExecuteASM(pCE) || !RunCopyExecuteASM(pCE)) {
		return false;
	}
	return true;
}
