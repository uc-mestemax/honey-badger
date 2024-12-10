interrupt_s struc
		interruptVector BYTE 0
		function DWORD 0
		rvalue QWORD 0
interrupt_s ends

backdoor_info_s struc
		status DWORD 0
		channelNum WORD 0
		padding WORD 0
		cookie1 DWORD 0
		cookie2 DWORD 0
		cmdLength DWORD 0
		padding2 DWORD 0
		pBuffer QWORD 0
		pEndBuffer QWORD 0
		replyLength DWORD 0
		replyID DWORD 0
backdoor_info_s ends

reg_set_s struc
		reg_rax QWORD 0
		reg_rcx QWORD 0
		reg_rdx QWORD 0
		reg_rbx QWORD 0
		reg_rbp QWORD 0
		reg_rsi QWORD 0
		reg_rdi QWORD 0
		reg_r8 QWORD 0
		reg_r9 QWORD 0
		reg_r10 QWORD 0
		reg_r11 QWORD 0
		reg_r12 QWORD 0
		reg_r13 QWORD 0
		reg_r14 QWORD 0
		reg_r15 QWORD 0
reg_set_s ends

run_custom_s struc
	regs reg_set_s <>
	pCode QWORD 0
	pSrc QWORD 0
	srcSize QWORD 0
run_custom_s ends

.data
testok    db 'RIP based addressing worked!', 0
testokdw  DWORD 4
.code

SIGNAL_INTERRUPT proc
	push rax
	mov al, (interrupt_s PTR [rcx]).interruptVector
	int 10h
	mov (interrupt_s PTR [rcx]).rvalue, rax
	pop rax
	ret
SIGNAL_INTERRUPT endp

GET_GLOBAL_TABLE_DESCRIPTOR proc
	sgdt [rcx]
	ret
GET_GLOBAL_TABLE_DESCRIPTOR endp

GET_INTERRUPT_TABLE_DESCRIPTOR proc
	sidt [rcx]
	ret
GET_INTERRUPT_TABLE_DESCRIPTOR endp

HYPERCALL proc
	vmcall
	ret
HYPERCALL endp

HYPERFUNCCALL proc
	vmfunc
	ret
HYPERFUNCCALL endp

CR3SWAP_READ proc
	push rdi
	mov rdi, cr3
	mov cr3, rcx
	mov rax, [rdx]
	mov cr3, rdi
	pop rdi
	ret
CR3SWAP_READ endp

PUSH_OVERWRITE_CONTEXT proc
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	push rcx
	mov rax, (reg_set_s PTR [rcx]).reg_rax
	push rax
	mov rax, rcx
	mov rcx, (reg_set_s PTR [rax]).reg_rcx
	mov rdx, (reg_set_s PTR [rax]).reg_rdx
	mov rbx, (reg_set_s PTR [rax]).reg_rbx
	mov rbp, (reg_set_s PTR [rax]).reg_rbp
	mov rsi, (reg_set_s PTR [rax]).reg_rsi
	mov rdi, (reg_set_s PTR [rax]).reg_rdi
	mov r8, (reg_set_s PTR [rax]).reg_r8
	mov r9, (reg_set_s PTR [rax]).reg_r9
	mov r10, (reg_set_s PTR [rax]).reg_r10
	mov r11, (reg_set_s PTR [rax]).reg_r11
	mov r12, (reg_set_s PTR [rax]).reg_r12
	mov r13, (reg_set_s PTR [rax]).reg_r13
	mov r14, (reg_set_s PTR [rax]).reg_r14
	mov r15, (reg_set_s PTR [rax]).reg_r15
	pop rax;Ret currently not supported
PUSH_OVERWRITE_CONTEXT endp

POP_RESTORE_CONTEXT proc
	push rax
	mov rax, [rsp+8h]
	mov (reg_set_s PTR [rax]).reg_rcx, rcx
	mov (reg_set_s PTR [rax]).reg_rdx, rdx
	mov (reg_set_s PTR [rax]).reg_rbx, rbx
	mov (reg_set_s PTR [rax]).reg_rbp, rbp
	mov (reg_set_s PTR [rax]).reg_rsi, rsi
	mov (reg_set_s PTR [rax]).reg_rdi, rdi
	mov (reg_set_s PTR [rax]).reg_r8, r8
	mov (reg_set_s PTR [rax]).reg_r9, r9
	mov (reg_set_s PTR [rax]).reg_r10, r10
	mov (reg_set_s PTR [rax]).reg_r11, r11
	mov (reg_set_s PTR [rax]).reg_r12, r12
	mov (reg_set_s PTR [rax]).reg_r13, r13
	mov (reg_set_s PTR [rax]).reg_r14, r14
	mov (reg_set_s PTR [rax]).reg_r15, r15
	mov rcx, rax
	pop rax
	mov (reg_set_s PTR [rcx]).reg_rax, rax
	pop rcx
	
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
	ret
POP_RESTORE_CONTEXT endp

PRIVILEGED_BD proc
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	push rcx
	mov rax, (reg_set_s PTR [rcx]).reg_rax
	push rax
	mov rax, rcx
	mov rcx, (reg_set_s PTR [rax]).reg_rcx
	mov rdx, (reg_set_s PTR [rax]).reg_rdx
	mov rbx, (reg_set_s PTR [rax]).reg_rbx
	mov rbp, (reg_set_s PTR [rax]).reg_rbp
	mov rsi, (reg_set_s PTR [rax]).reg_rsi
	mov rdi, (reg_set_s PTR [rax]).reg_rdi
	mov r8, (reg_set_s PTR [rax]).reg_r8
	mov r9, (reg_set_s PTR [rax]).reg_r9
	mov r10, (reg_set_s PTR [rax]).reg_r10
	mov r11, (reg_set_s PTR [rax]).reg_r11
	mov r12, (reg_set_s PTR [rax]).reg_r12
	mov r13, (reg_set_s PTR [rax]).reg_r13
	mov r14, (reg_set_s PTR [rax]).reg_r14
	mov r15, (reg_set_s PTR [rax]).reg_r15
	pop rax

	in eax, dx
	
	jmp POP_RESTORE_CONTEXT
PRIVILEGED_BD endp

JMP_CUSTOM proc
	
JMP_CUSTOM endp

RUN_CUSTOM proc
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	push rcx
	mov rax, (reg_set_s PTR [rcx]).reg_rax
	sub rsp, 010h
	push rax
	mov rax, rcx
	mov rcx, (reg_set_s PTR [rax]).reg_rcx
	mov rdx, (reg_set_s PTR [rax]).reg_rdx
	mov rbx, (reg_set_s PTR [rax]).reg_rbx
	mov rbp, (reg_set_s PTR [rax]).reg_rbp
	mov rsi, (reg_set_s PTR [rax]).reg_rsi
	mov rdi, (reg_set_s PTR [rax]).reg_rdi
	mov r8, (reg_set_s PTR [rax]).reg_r8
	mov r9, (reg_set_s PTR [rax]).reg_r9
	mov r10, (reg_set_s PTR [rax]).reg_r10
	mov r11, (reg_set_s PTR [rax]).reg_r11
	mov r12, (reg_set_s PTR [rax]).reg_r12
	mov r13, (reg_set_s PTR [rax]).reg_r13
	mov r14, (reg_set_s PTR [rax]).reg_r14
	mov r15, (reg_set_s PTR [rax]).reg_r15

	mov rax, (run_custom_s PTR [rax]).pCode
	mov [rsp+08h], rax
	lea rax, cool
	mov [rsp+010h], rax
	pop rax
	ret

cool:
	jmp POP_RESTORE_CONTEXT
RUN_CUSTOM endp

RUN_CUSTOM_FORCE_INTERP proc
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	push rcx
	mov rax, (reg_set_s PTR [rcx]).reg_rax
	sub rsp, 010h
	push rax
	mov rax, rcx
	mov rcx, (reg_set_s PTR [rax]).reg_rcx
	mov rdx, (reg_set_s PTR [rax]).reg_rdx
	mov rbx, (reg_set_s PTR [rax]).reg_rbx
	mov rbp, (reg_set_s PTR [rax]).reg_rbp
	mov rsi, (reg_set_s PTR [rax]).reg_rsi
	mov rdi, (reg_set_s PTR [rax]).reg_rdi
	mov r8, (reg_set_s PTR [rax]).reg_r8
	mov r9, (reg_set_s PTR [rax]).reg_r9
	mov r10, (reg_set_s PTR [rax]).reg_r10
	mov r11, (reg_set_s PTR [rax]).reg_r11
	mov r12, (reg_set_s PTR [rax]).reg_r12
	mov r13, (reg_set_s PTR [rax]).reg_r13
	mov r14, (reg_set_s PTR [rax]).reg_r14
	mov r15, (reg_set_s PTR [rax]).reg_r15

	mov rax, (run_custom_s PTR [rax]).pCode
	mov [rsp+08h], rax
	lea rax, cool
	mov [rsp+010h], rax
	pop rax
	call DO_TESTING
	ret

cool:
	jmp POP_RESTORE_CONTEXT
RUN_CUSTOM_FORCE_INTERP endp


BIOS_INTERRUPT_TEST proc
	call PUSH_OVERWRITE_CONTEXT

	int 10h
	
	call POP_RESTORE_CONTEXT
	ret
BIOS_INTERRUPT_TEST endp

DISABLE_NMI proc
	push rdx
	push rax
	mov dx, 070h
	in al, dx
	or al, 080h
	out 070h, al
	pop rax
	pop rdx
	ret
DISABLE_NMI endp

ENABLE_NMI proc
	push rdx
	push rax
	mov dx, 070h
	in al, dx
	and al, 07Fh
	out 070h, al
	pop rax
	pop rdx
	ret
ENABLE_NMI endp

RUN_REALMODE proc
	push rdx
	push rax
	cli
	call DISABLE_NMI
	
	mov rax, cr0
	and rax, 07FFFFFFEh
	mov cr0, rax



	or rax, 080000001h
	mov cr0, rax

	call ENABLE_NMI
	sti
	pop rax
	pop rdx
	ret
RUN_REALMODE endp

DO_TESTING proc
	push rax
	push rdx
	push rcx
	push rbx
	
	mov rax, 0564D5868h
	mov rdx, 05658h
	mov rcx, 03h
	shl rcx, 16
	or rcx, 010h
	mov rbx, 0FFFh
	in eax, dx

	pop rbx
	pop rcx
	pop rdx
	pop rax
	vmcall ;Trigger VMExit to interpreter
	ret
DO_TESTING endp

AsmEnableVmxOperation proc

	PUSH RAX			    ; Save the state
	
	XOR RAX, RAX			; Clear the RAX
	MOV RAX, CR4

	OR RAX,02000h	    	; Set the 14th bit
	MOV CR4, RAX
	
	POP RAX			     	; Restore the state
	RET

AsmEnableVmxOperation endp

end