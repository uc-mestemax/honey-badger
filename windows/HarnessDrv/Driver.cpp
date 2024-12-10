#include <ntifs.h>
#include <ntstrsafe.h>
#include <ntddk.h>
#include <intrin.h>

#define IOCTL_IO_UNLOAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_GTD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_ITD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_IMAGE_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MEM_ACTION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PREPARE_COPY_EXECUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2007, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RUN_COPY_EXECUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2008, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define CALC_PCI_BusAddress(b,d,f,o)(0x80000000 | b << 16 | d << 11 | f << 8 | o)

typedef unsigned char BYTE;

struct interrupt_s {
	BYTE interruptVector;
	DWORD function;
};

struct get_vmcs_value_s {
	ULONG_PTR paVMCS;
	unsigned int field;
};

struct reg_set_s {
	long long reg_rax;
	long long reg_rcx;
	long long reg_rdx;
	long long reg_rbx;
	//long long reg_rsp;
	long long reg_rbp;
	long long reg_rsi;
	long long reg_rdi;
	long long reg_r8;
	long long reg_r9;
	long long reg_r10;
	long long reg_r11;
	long long reg_r12;
	long long reg_r13;
	long long reg_r14;
	long long reg_r15;
};



struct copy_execute_s {
	reg_set_s set;
	void* pFinalCode;
	void* pSrc;
	size_t size;
	unsigned int numArgs;
	void* args[];
};

#pragma pack(push, 1)
struct idtr_s {
	UINT16 limit;
	UINT64 base;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct gdtr_s {
	UINT16 limit;
	UINT64 base;
};
#pragma pack(pop)

extern "C" {
	void SIGNAL_INTERRUPT(interrupt_s* pInterruptInfo);
	void GET_GLOBAL_TABLE_DESCRIPTOR(gdtr_s* pOut);
	void GET_INTERRUPT_TABLE_DESCRIPTOR(idtr_s* pOut);
	void HYPERCALL(unsigned int hcID, void* pIn, void* pOut);
	void PRIVILEGED_BD(reg_set_s* pIn);
	void BIOS_INTERRUPT_TEST(reg_set_s* pIn);
	UINT64 CR3SWAP_READ(UINT64 CR3, void* pRead);
	void RUN_REALMODE();
	void DO_TESTING();
	void RUN_CUSTOM(copy_execute_s* pCE);
	void AsmEnableVmxOperation();
	void RUN_CUSTOM_FORCE_INTERP(copy_execute_s* pCE);
}

UNICODE_STRING NAME = RTL_CONSTANT_STRING(L"\\HarnessDrv");
UNICODE_STRING DRIVER_NAME = RTL_CONSTANT_STRING(L"\\Driver\\HarnessDrv");
UNICODE_STRING DEVICE_NAME = RTL_CONSTANT_STRING(L"\\Device\\HarnessDrv");
UNICODE_STRING DEVICE_SYMBOLIC_NAME = RTL_CONSTANT_STRING(L"\\??\\HarnessDrvLink");
void* gpVMCS = NULL;

extern "C" { //undocumented windows internal functions (exported by ntoskrnl)
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
	/*NTKERNELAPI NTSTATUS NtQueryVirtualMemory(
		HANDLE ProcessHandle,
		PVOID BaseAddress,
		MEMORY_INFORMATION_CLASS MemoryInformationClass,
		PVOID MemoryInformation,
		SIZE_T MemoryInformationLength,
		PSIZE_T ReturnLength);*/
}

struct info_t { //message type that will be passed between user program and driver
	HANDLE target_pid = 0; //process id of process we want to read from / write to
	void* target_address = 0x0; //address in the target proces we want to read from / write to
	void* buffer_address = 0x0; //address in our usermode process to copy to (read mode) / read from (write mode)
	SIZE_T size = 0; //size of memory to copy between our usermode process and target process
	SIZE_T return_size = 0; //number of bytes successfully read / written
};

VOID DriverUnload(PDRIVER_OBJECT driver_obj) {
	UNREFERENCED_PARAMETER(driver_obj);
	DbgPrint("Removing Device\n");
	IoDeleteSymbolicLink(&DEVICE_SYMBOLIC_NAME);
	if (driver_obj->DeviceObject) {
		IoDeleteDevice(driver_obj->DeviceObject);
	}
}

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

LARGE_INTEGER toLargeInteger(long long value)
{
	LARGE_INTEGER result;
	result.QuadPart = value;
	return result;
};

typedef struct _SYSTEM_MODULE_ENTRY
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} SYSTEM_MODULE_ENTRY, * PSYSTEM_MODULE_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG Count;
	SYSTEM_MODULE_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemModuleInformation = 11
} SYSTEM_INFORMATION_CLASS;

#pragma pack(push, 1)
typedef struct _MAP_PHY_MEM {
	DWORD_PTR Size;
	DWORD_PTR PhysicalAddress;
	HANDLE SectionHandle;
	void* BaseAddress;
	void* ReferenceObject;
} MAP_PHY_MEM, * PMAP_PHY_MEM;
#pragma pack(pop)

typedef struct _INTERRUPT_DESCRIPTOR_TABLE
{
	UINT16 Offset0;
	UINT16 Unused0;
	UINT8 Unused1;
	UINT8 Unused2;
	UINT16 Offset1;
} INTERRUPT_DESCRIPTOR_TABLE, * PINTERRUPT_DESCRIPTOR_TABLE;

typedef struct _INTERRUPT_DESCRIPTOR_TABLE_REGISTER
{
	UINT16 Unused0;
	PINTERRUPT_DESCRIPTOR_TABLE InterruptDescriptorTable;
} INTERRUPT_DESCRIPTOR_TABLE_REGISTER, * PINTERRUPT_DESCRIPTOR_TABLE_REGISTER;


extern "C" {
	NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass, OUT PVOID SystemInformation, IN ULONG SystemInformationLength, OUT PULONG ReturnLength OPTIONAL);
}

NTKERNELAPI PVOID (*pfnPsGetProcessSectionBaseAddress)(__in PEPROCESS Process);
PVOID GetImageBaseAddress(const char* pStr) {

	DWORD bytes = 0;
	PVOID imageBase = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, 0, sizeof(bytes), &bytes);

	PSYSTEM_MODULE_INFORMATION pMods = (PSYSTEM_MODULE_INFORMATION)ExAllocatePool(NonPagedPool, bytes);
	RtlZeroMemory(pMods, bytes);

	status = ZwQuerySystemInformation(SystemModuleInformation, pMods, bytes, &bytes);

	if (status == STATUS_SUCCESS) {
		PSYSTEM_MODULE_ENTRY pMod = pMods->Module;
		STRING targetModuleName;
		RtlInitAnsiString(&targetModuleName, pStr);
		STRING current;
		for (ULONG i = 0; i < pMods->Count; i++)
		{
			RtlInitAnsiString(&current, (PCSZ)pMod[i].FullPathName);
			DbgPrint("Module %d: %s\n", i, pMod[i].FullPathName);
			if (0 == RtlCompareString(&targetModuleName, &current, TRUE))
			{
				
				imageBase = pMod[i].ImageBase;
				break;
			}
		}
	}
	ExFreePool(pMods);
	return imageBase;

}

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	PULONG_PTR Base;
	PULONG Count;
	ULONG Limit;
} KSERVICE_TABLE_DESCRIPTOR, * PKSERVICE_TABLE_DESCRIPTOR;

void io_send_string_out(UINT8 repSize, USHORT portOut, PVOID pOut, ULONG outSize) {
	switch (repSize) {
	case 1:__outbytestring(portOut, (PUCHAR)pOut, outSize); break;
	case 2:__outwordstring(portOut, (PUSHORT)pOut, outSize); break;
	case 4:__outdwordstring(portOut, (PULONG)pOut, outSize); break;
	default:DbgPrint("Invalid rep size passed to io send out\n"); break;
	}
}

void io_receive_string_in(UINT8 repSize, USHORT portIn, PVOID pIn, ULONG inSize) {
	switch (repSize) {
	case 1:__inbytestring(portIn, (PUCHAR)pIn, inSize); break;
	case 2:__inwordstring(portIn, (PUSHORT)pIn, inSize); break;
	case 4:__indwordstring(portIn, (PULONG)pIn, inSize); break;
	default:DbgPrint("Invalid rep size passed to io receive in\n"); break;
	}
}

bool verify_ioctl_sizes(PIRP Irp, size_t szIn, size_t szOut) {
	PIO_STACK_LOCATION pIOSL = IoGetCurrentIrpStackLocation(Irp);
	size_t szBufferIn = pIOSL->Parameters.DeviceIoControl.InputBufferLength;
	size_t szBufferOut = pIOSL->Parameters.DeviceIoControl.OutputBufferLength;
	if (szBufferIn >= szIn && szBufferOut >= szOut) {
		return true;
	}
	else {
		Irp->IoStatus.Status = -87;
		return false;
	}
}

void reg_action(REG_CONSTANT reg, bool write, long long newValue, long long* pOldValue) {
	long long regValue = NULL;
	switch (reg) {
	case REG_CR0:
		regValue = __readcr0();
		if (write) { __writecr0(newValue); };
		break;
	case REG_CR1:
		//Not used
		break;
	case REG_CR2:
		regValue = __readcr2();
		if (write) { __writecr2(newValue); };
		break;
	case REG_CR3:
		regValue = __readcr3();
		if (write) { __writecr3(newValue); };
		break;
	case REG_CR4:
		regValue = __readcr4();
		if (write) { __writecr4(newValue); };
		break;
	case REG_CR5:
	case REG_CR6:
	case REG_CR7:
		break;
	case REG_CR8:
		regValue = __readcr8();
		if (write) { __writecr8(newValue); };
		break;
	default:
		break;
	}
	*pOldValue = regValue;
}

static
PINTERRUPT_DESCRIPTOR_TABLE
GetCurrentProcessorInterruptDescriptorTable(VOID)
{
	void* Register;

	__sidt(&Register);

	return (PINTERRUPT_DESCRIPTOR_TABLE)Register;
}

static BOOLEAN MapPhysicalMemory(HANDLE PhysicalMemory,
	PUINT64 Address, PSIZE_T Length,
	PVOID* VirtualAddress)
{
	NTSTATUS            ntStatus;
	PHYSICAL_ADDRESS    viewBase;
	char                error[256];

	*VirtualAddress = 0;
	viewBase.QuadPart = (ULONGLONG)(*Address);
	ntStatus = ZwMapViewOfSection(PhysicalMemory,
		(HANDLE)-1,
		VirtualAddress,
		0L,
		*Length,
		&viewBase,
		Length,
		ViewShare,
		0,
		PAGE_READONLY);

	if (!NT_SUCCESS(ntStatus)) {

		sprintf_s(error, "Could not map view of %X length %X",
			*Address, *Length);
		DbgPrint(error, ntStatus);
		return FALSE;
	}

	*Address = viewBase.LowPart;
	return TRUE;
}

static HANDLE OpenPhysicalMemory()
{
	NTSTATUS        status;
	HANDLE          physmem;
	UNICODE_STRING  physmemString;
	OBJECT_ATTRIBUTES attributes;
	WCHAR           physmemName[] = L"\\device\\physicalmemory";

	RtlInitUnicodeString(&physmemString, physmemName);

	InitializeObjectAttributes(&attributes, &physmemString,
		OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenSection(&physmem, SECTION_MAP_READ, &attributes);

	if (!NT_SUCCESS(status)) {

		DbgPrint("Could not open \\device\\physicalmemory", status);
		return NULL;
	}

	return physmem;
}

long long msr_action(unsigned int index, bool write, long long newValue) {
	long long regValue = __readmsr(index);
	DbgPrint("Read %p --> %p\n", index, regValue);
	if (write) { __writemsr(index, newValue); };
	return regValue;
}

void* mapVirtualAddress(void* pVirtualAddress, size_t size) {
	PMDL newMDL = IoAllocateMdl(pVirtualAddress, size, NULL, NULL, NULL);
	MmBuildMdlForNonPagedPool(newMDL);
	return MmMapLockedPagesSpecifyCache(newMDL, UserMode, MmNonCached, NULL, FALSE, LowPagePriority);
}

void* alloc_kmem(void* pVAddress, size_t size) {
	PHYSICAL_ADDRESS p = { MAXULONG64 };
	PVOID start = MmAllocateContiguousMemory(size, p);
	PMDL newMDL = IoAllocateMdl(start, size, NULL, NULL, NULL);
	if (newMDL) {
		MmBuildMdlForNonPagedPool(newMDL);
		PVOID f = MmMapLockedPagesSpecifyCache(newMDL, KernelMode, MmNonCached, pVAddress, FALSE, LowPagePriority);
		if (f) {
			DbgPrint("Succeeded %p", f);
		}
		else {
			DbgPrint("Failed f");
		}
	}
	else {
		DbgPrint("Failed p");
	}

	return pVAddress;
}

static
LONG
GetInterruptServiceRoutine(PINTERRUPT_DESCRIPTOR_TABLE Table, ULONG InterruptNumber)
{
	return (LONG)((Table[InterruptNumber].Offset1 << 16) | Table[InterruptNumber].Offset0);
}

struct swap_read_test_s {
	ULONG64 cr3;
	void* pToRead;
};

void (*gpfExecute)(...) = NULL;

ULONG_PTR CustomExecuteDispatch(copy_execute_s* pCE) {
	//DbgPrint("gpfExecute %p %p %p\n", gpfExecute, pCE->numArgs, pCE->args[0]);

	switch (pCE->numArgs) {
	case 0:gpfExecute(); break;
	case 1:gpfExecute(pCE->args[0]); break;
	case 2:gpfExecute(pCE->args[0], pCE->args[1]); break;
	case 3:gpfExecute(pCE->args[0], pCE->args[1], pCE->args[2]); break;
	case 4:gpfExecute(pCE->args[0], pCE->args[1], pCE->args[2], pCE->args[3]); break;
	case 5:gpfExecute(pCE->args[0], pCE->args[1], pCE->args[2], pCE->args[3], pCE->args[4]); break;
	default:
		DbgPrint("Too many arguments supplied\n");
		break;
	}
	return 1;
}

ULONG_PTR CustomExecuteIPIWorker(_In_ ULONG_PTR Argument) {
	copy_execute_s* pCE = (copy_execute_s*)Argument;
	CustomExecuteDispatch(pCE);
	return 1;
}

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

bool HandleMemAction(MemActionHdr_s* pHdr) {
	if (pHdr->type == MEMACTION_ALLOC_CONTIGUOUS) {
		MemAction_AllocContiguous* pAlloc = (MemAction_AllocContiguous*)pHdr;
		PVOID addr = MmAllocateContiguousMemory(pAlloc->size, pAlloc->highestAddress);
		if (addr) {
			RtlFillMemory(addr, pAlloc->size, 0x0);
			pAlloc->h.result.kVA = addr;
			pAlloc->h.result.size = pAlloc->size;
			if (pAlloc->mapUVA) {
				pAlloc->h.result.uVA = mapVirtualAddress(addr, pAlloc->size);
			}
			pAlloc->h.result.physAddress = MmGetPhysicalAddress(addr);
		}
		else {
			return false;
		}
	}
	else if (pHdr->type == MEMACTION_FREE_CONTIGUOUS) {
		MemAction_FreeContiguous* pFree = (MemAction_FreeContiguous*)pHdr;
		MmFreeContiguousMemory(pFree->pVA);
	}
	else if (pHdr->type == MEMACTION_MAP_PHYS) {
		MemAction_MapPhys* pMap = (MemAction_MapPhys*)pHdr;
		PVOID addr = MmMapIoSpace(pMap->phys, pMap->size, MmNonCached);
		if (addr) {
			MmSecureVirtualMemory(addr, pMap->size, PAGE_READWRITE);
			pMap->h.result.physAddress = pMap->phys;
			pMap->h.result.size = pMap->size;
			pMap->h.result.kVA = addr;
			if (pMap->mapUVA) {
				pMap->h.result.uVA = mapVirtualAddress(addr, pMap->size);
			}
		}
		else {
			return false;
		}
	}
	else if (pHdr->type == MEMACTION_GET_VIRTUAL) {
		MemAction_GetVA* pGet = (MemAction_GetVA*)pHdr;
		pGet->h.result.kVA = MmGetVirtualForPhysical(pGet->phys);
	}
	else if (pHdr->type == MEMACTION_GET_PHYSICAL) {
		MemAction_GetPhys* pGet = (MemAction_GetPhys*)pHdr;
		pGet->h.result.physAddress = MmGetPhysicalAddress(pGet->pVA);
	}
	else if (pHdr->type == MEMACTION_MAP_VA) {
		MemAction_MapVA* pMap = (MemAction_MapVA*)pHdr;
		pMap->h.result.uVA = mapVirtualAddress(pMap->pVA, pMap->size);
	}
	else if (pHdr->type == MEMACTION_ALLOC_KVIRTUAL) {
		MemAction_AllocKVA* pKAlloc = (MemAction_AllocKVA*)pHdr;
		/*void* pAllocated = alloc_kmem(pKAlloc->pKAllocated, pKAlloc->size);
		if (pAllocated) {
			*(void**)Irp->AssociatedIrp.SystemBuffer = pAllocated;
			Irp->IoStatus.Information = sizeof(void*);
		}
		else {
			Irp->IoStatus.Status = -55;
		}*/
		return false;
	}
	else if (pHdr->type == MEMACTION_RW) {
		MemAction_RW* pRW = (MemAction_RW*)pHdr;
		memcpy(pRW->pDst, pRW->pSrc, pRW->size);
	}
	else if (pHdr->type == MEMACTION_MMSECUREV) {
		MemAction_MMSecureV* pSecure = (MemAction_MMSecureV*)pHdr;
		if (!MmSecureVirtualMemory(pSecure->va, pSecure->size, pSecure->rw ? PAGE_READWRITE : PAGE_READONLY)) {
			return false;
		}
	}
	else if (pHdr->type == MEMACTION_ALLOC_EXECUTABLE) {
		MemAction_AllocContiguous* pAlloc = (MemAction_AllocContiguous*)pHdr;
		PVOID addr = ExAllocatePool(NonPagedPool, pAlloc->size);
		if (addr) {
			RtlFillMemory(addr, pAlloc->size, 0x0);
			pAlloc->h.result.kVA = addr;
			pAlloc->h.result.size = pAlloc->size;
			if (pAlloc->mapUVA) {
				pAlloc->h.result.uVA = mapVirtualAddress(addr, pAlloc->size);
			}
			pAlloc->h.result.physAddress = MmGetPhysicalAddress(addr);
		}
	}
	else {
		DbgPrint("Mem Action not supported\n");
		return false;
	}
	return true;
}

bool verify_mem_action_ioctl_size(MemActionHdr_s* pHdr, size_t inSize) {
	return true;
	size_t expectedSize = 0;
	switch (pHdr->type) {
	case MEMACTION_ALLOC_CONTIGUOUS: expectedSize = sizeof(MemAction_AllocContiguous);
	case MEMACTION_FREE_CONTIGUOUS: expectedSize = sizeof(MemAction_FreeContiguous);
	case MEMACTION_MAP_PHYS: expectedSize = sizeof(MemAction_MapPhys);
	case MEMACTION_GET_VIRTUAL: expectedSize = sizeof(MemAction_GetVA);
	case MEMACTION_GET_PHYSICAL: expectedSize = sizeof(MemAction_GetPhys);
	case MEMACTION_MAP_VA: expectedSize = sizeof(MemAction_MapVA);
	case MEMACTION_ALLOC_KVIRTUAL: expectedSize = sizeof(MemAction_AllocKVA);
	case MEMACTION_RW: expectedSize = sizeof(MemAction_RW);
	case MEMACTION_MMSECUREV: expectedSize = sizeof(MemAction_MMSecureV);
	case MEMACTION_ALLOC_EXECUTABLE: expectedSize = sizeof(MemAction_AllocContiguous);
	default: return false;
	}
	return inSize >= expectedSize;
}

bool SetupCustomExecute(copy_execute_s* pCE) {
	gpfExecute = (void(*)(...))ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, pCE->size, 'copy');
	if (gpfExecute) {
		memcpy(gpfExecute, pCE->pSrc, pCE->size);
		pCE->pFinalCode = gpfExecute;
		return true;
	}
	else {
		DbgPrint("Custom allocation failure\n");
		return false;
	}
}

NTSTATUS HandleCustomIOCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	PIO_STACK_LOCATION stackLocation = NULL;

	stackLocation = IoGetCurrentIrpStackLocation(Irp);
	size_t szBufferIn = stackLocation->Parameters.DeviceIoControl.InputBufferLength;
	size_t szBufferOut = stackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID bufferOut = Irp->UserBuffer;
	ULONG code = stackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (code) {
	case IOCTL_MSR:
		if (verify_ioctl_sizes(Irp, sizeof(msr_action_s), sizeof(long long))) {
			msr_action_s* action = (msr_action_s*)Irp->AssociatedIrp.SystemBuffer;
			*(long long*)Irp->AssociatedIrp.SystemBuffer = msr_action(action->index, action->isWrite, action->value);
			Irp->IoStatus.Information = sizeof(long long);
		}
		break;
	case IOCTL_GET_GTD:
		if (verify_ioctl_sizes(Irp, 0, sizeof(gdtr_s))) {
			GET_GLOBAL_TABLE_DESCRIPTOR((gdtr_s*)Irp->AssociatedIrp.SystemBuffer);
			Irp->IoStatus.Information = sizeof(gdtr_s);
		}
		break;
	case IOCTL_GET_ITD:
		if (verify_ioctl_sizes(Irp, 0, sizeof(idtr_s))) {
			__sidt(Irp->AssociatedIrp.SystemBuffer);
			Irp->IoStatus.Information = sizeof(idtr_s);
		}
		break;
	case IOCTL_GET_IMAGE_BASE:
		if (verify_ioctl_sizes(Irp, sizeof(char*), sizeof(PVOID*))) {
			PVOID imageBase = GetImageBaseAddress(*(char**)Irp->AssociatedIrp.SystemBuffer);
			*(PVOID*)Irp->AssociatedIrp.SystemBuffer = imageBase;
			Irp->IoStatus.Information = sizeof(PVOID*);
		}
		break;
	case IOCTL_MEM_ACTION:
		if (verify_ioctl_sizes(Irp, sizeof(MemActionHdr_s*), 0)) {
			PIO_STACK_LOCATION pIOSL = IoGetCurrentIrpStackLocation(Irp);
			MemActionHdr_s* pHdr = *(MemActionHdr_s**)Irp->AssociatedIrp.SystemBuffer;
			if (verify_mem_action_ioctl_size(pHdr, pIOSL->Parameters.DeviceIoControl.InputBufferLength)) {
				Irp->IoStatus.Status = HandleMemAction(pHdr) ? STATUS_SUCCESS : -87;
			}
			else {
				Irp->IoStatus.Status = -87;
			}
		}
		else {
			Irp->IoStatus.Status = -87;
		}

		break;
	case IOCTL_IO_UNLOAD:
		//ZwUnloadDriver(DeviceObject->DriverObject->HardwareDatabase);
		DriverUnload(NULL);
		break;
	case IOCTL_PREPARE_COPY_EXECUTE:
		if (verify_ioctl_sizes(Irp, sizeof(copy_execute_s*), 0)) {
			copy_execute_s* pCE = *(copy_execute_s**)Irp->AssociatedIrp.SystemBuffer;
			if (SetupCustomExecute(pCE)) {

			}
			else {
				DbgPrint("Failed to setup execute\n");
				Irp->IoStatus.Status = -87;
			}
		}
		else {
			DbgPrint("Invalid arguments\n");
			Irp->IoStatus.Status = -87;
		}
		break;
	case IOCTL_RUN_COPY_EXECUTE:
		if (verify_ioctl_sizes(Irp, sizeof(copy_execute_s*), 0)) {
			copy_execute_s* pCE = *(copy_execute_s**)Irp->AssociatedIrp.SystemBuffer;
			if (gpfExecute) {
				CustomExecuteDispatch(pCE);
			}
			else {
				DbgPrint("Code not prepared\n");
				Irp->IoStatus.Status = -87;
			}
		}
		else {
			DbgPrint("Invalid arguments\n");
			Irp->IoStatus.Status = -87;
		}
		break;
	default: {
		DbgPrint("INVALID IOCTL\n");
		Irp->IoStatus.Status = -87;
		break;
	}
	}
CompleteRequest:
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS unsupported_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS create_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS close_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

extern "C" {
	NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
		PDEVICE_OBJECT dev_obj;

		NTSTATUS status = IoCreateDevice(driver_obj, 0, &DEVICE_NAME, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev_obj);
		if (status != STATUS_SUCCESS) return status;

		status = IoCreateSymbolicLink(&DEVICE_SYMBOLIC_NAME, &DEVICE_NAME);
		if (status != STATUS_SUCCESS) return status;

		SetFlag(dev_obj->Flags, DO_BUFFERED_IO); //set DO_BUFFERED_IO bit to 1*/

		for (int t = 0; t <= IRP_MJ_MAXIMUM_FUNCTION; t++) //set all MajorFunction's to unsupported
			driver_obj->MajorFunction[t] = unsupported_io;

		//then set supported functions to appropriate handlers
		driver_obj->MajorFunction[IRP_MJ_CREATE] = create_io; //link our io create function
		driver_obj->MajorFunction[IRP_MJ_CLOSE] = close_io; //link our io close function
		driver_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HandleCustomIOCTL; //link our control code handler
		driver_obj->DriverUnload = DriverUnload;

		ClearFlag(dev_obj->Flags, DO_DEVICE_INITIALIZING); //set DO_DEVICE_INITIALIZING bit to 0 (we are done initializing)

		return STATUS_SUCCESS;
	}
}