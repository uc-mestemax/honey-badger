.data

.code

SepCreateTokenExHook proc
	add rsp, 02A0h
	pop r15
	pop r14
	pop r13
	pop r12
	pop rdi
	pop rsi
	pop rbp
	nop
	nop
	nop
	nop
	nop
	ret
SepCreateTokenExHook endp

ObfDereferenceObjectWithTagHook proc
	nop
	nop
	nop
	nop
	nop
	ret
ObfDereferenceObjectWithTagHook endp

KiPageFaultHook proc
	nop
	nop
	nop
	nop
	nop
	ret
KiPageFaultHook endp

end