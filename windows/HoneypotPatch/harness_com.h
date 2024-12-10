#pragma once
#include "common.h"
#include "descriptor_tables.h"

#define IOCTL_IO_UNLOAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_GTD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_ITD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_IMAGE_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MEM_ACTION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PREPARE_COPY_EXECUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2007, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RUN_COPY_EXECUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2008, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef uint64_t PHYSICAL_ADDRESS, * PPHYSICAL_ADDRESS;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned short ushort;

struct vmx_io {
	vmx_io* pNextRequest;
	bool sendOut;
	USHORT port;
	ULONG count;
	PUCHAR pBuff;
	UINT8 repSize;
};

struct vmx_io_norep {
	vmx_io* pNextRequest;
	bool sendOut;
	USHORT port;
	BYTE dataSize;
	DWORD value;
};

struct mapped_phys_mem {
	void* pUVA;
	void* pKVA;
	PHYSICAL_ADDRESS physMem;
};

enum REG_CONSTANT {
	REG_CR0,
	REG_CR1,
	REG_CR2,
	REG_CR3,
	REG_CR4,
	REG_CR5,
	REG_CR6,
	REG_CR7,
	REG_CR8,
	REG_EFER,
	REG_XCR0,
	REG_XSS
};

enum MSR_CONSTANT {

};

struct modify_reg {
	REG_CONSTANT reg;
	long long value;
	bool isWrite;
};

struct msr_action_s {
	unsigned int index;
	long long value;
	bool isWrite;
};

struct hyercall_s {
	unsigned int hcID;
	void* pIn;
	void* pOut;
};

struct reg_set_s {
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbx;
	//uint64_t reg_rsp;
	uint64_t rbp;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
};

struct sync_test_arg {
	void* pMsg;
	size_t msgSize;
	void* pMsg2;
	size_t msgSize2;
	volatile UINT64* ptrOverwrite;
};

struct swap_read_test_s {
	ULONG64 cr3;
	uint64_t pToRead;
};

enum MemActionType {
	MEMACTION_ALLOC_CONTIGUOUS,
	MEMACTION_FREE_CONTIGUOUS,
	MEMACTION_MAP_PHYS,
	MEMACTION_GET_VIRTUAL,
	MEMACTION_GET_PHYSICAL,
	MEMACTION_MAP_VA,
	MEMACTION_ALLOC_KVIRTUAL,
	MEMACTION_RW,
	MEMACTION_MMSECUREV,
	MEMACTION_ALLOC_EXECUTABLE
};

struct MemActionResult_s {
	PHYSICAL_ADDRESS physAddress;
	void* kVA;
	void* uVA;
	size_t size;
};

struct MemActionHdr_s {
	MemActionType type;
	MemActionResult_s result;
};

struct MemAction_AllocContiguous {
	MemActionHdr_s h;
	size_t size;
	PHYSICAL_ADDRESS highestAddress;
	bool mapUVA;
};

struct MemAction_FreeContiguous {
	MemActionHdr_s h;
	void* pVA;
};

struct MemAction_MapPhys {
	MemActionHdr_s h;
	PHYSICAL_ADDRESS phys;
	size_t size;
	bool mapUVA;
};

struct MemAction_GetPhys {
	MemActionHdr_s h;
	void* pVA;
};

struct MemAction_GetVA {
	MemActionHdr_s h;
	PHYSICAL_ADDRESS phys;
};

struct MemAction_MapVA {
	MemActionHdr_s h;
	void* pVA;
	size_t size;
};

struct MemAction_AllocKVA {
	MemActionHdr_s h;
	size_t size;
	bool mapUVA;
};

struct MemAction_RW {
	MemActionHdr_s h;
	void* pSrc;
	void* pDst;
	size_t size;
};

struct MemAction_MMSecureV {
	MemActionHdr_s h;
	PVOID va;
	size_t size;
	bool rw;
};

struct copy_execute_s {
	reg_set_s set;
	void* pFinalCode;
	void* pSrc;
	size_t size;
	unsigned int numArgs;
	void* args[];
};

struct get_vmcs_value_s {
	ULONG_PTR paVMCS;
	unsigned int field;
};

extern HANDLE ghHarnessDrv;

mapped_phys_mem* AllocContiguous(size_t size, uint64_t highestAddress, bool mapUVA);
mapped_phys_mem* AllocAndMapContiguous(size_t size, uint64_t highestAddress);
void* userMapPhys(PHYSICAL_ADDRESS address, size_t bytes);
void* kernelMapPhys(PHYSICAL_ADDRESS address, size_t size);
uint64_t msr_action(uint index, long long newValue, bool write);
PHYSICAL_ADDRESS get_physical_address(void* baseAddress);
bool rw_kernel(void* pSrc, void* pDst, size_t size);
bool get_GTD(struct gdtr_s*);
bool get_ITD(struct idtr_s* pIDTR);
uint64_t swap_read_test(swap_read_test_s* pSyncArg);
void do_vmm_testing();
void* kvirtual_alloc(void* pVirtual, size_t sz);
void* GetImageBase(char* pImageName);
bool unloadHarnessDrv();
void OpenHarnessDrv();
bool MMSecureMem(void* pAddress, size_t size, bool rw);
mapped_phys_mem* AllocExecutable(size_t size, bool mapUVA);
copy_execute_s* SetupCustomExecute(void* pFunction, size_t size, uint numArgs, ...);
bool PrepareCopyExecuteASM(copy_execute_s* pCE);
bool RunCopyExecuteASM(copy_execute_s* pCE);
bool CopyExecuteASM(copy_execute_s* pCE);