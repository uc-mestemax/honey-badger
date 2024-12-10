#pragma once
#pragma once
#include <Windows.h>
#include "common_types.h"

#pragma pack(push, 1)
struct idtr_s {
	uint16_t limit;
	uint64_t base;
};
#pragma pack(pop)

struct idt64_descriptor_s {
	uint16_t offset_1;        // offset bits 0..15
	uint16_t selector;        // a code segment selector in GDT or LDT
	uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
	uint8_t  type_attributes; // gate type, dpl, and p fields
	uint16_t offset_2;        // offset bits 16..31
	uint32_t offset_3;        // offset bits 32..63
	uint32_t zero;            // reserved
};

#pragma pack(push, 1)
struct gdtr_s {
	uint16_t limit;
	uint64_t base;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct gdt_entry_access_t {
	uint8_t accessed : 1; //CPU sets this to 1 if the segment is accessed
	uint8_t read_write : 1; //On code segments, is segment readable (can never write), on data segments, is segment writeable (can awlays read)
	uint8_t direction : 1; //On code segments, if 1, code can be executed at lower privilege. On data segments, if 1 segment grows down (ie offset has to be greater than base).
	uint8_t executable : 1; //If 1 code segment, if 0, data segment.
	uint8_t app : 1; //0 if system, 1 if application
	uint8_t privilege : 2; //Privilege level, 0-3, 0 highest.
	uint8_t present : 1; //Must be 1 for valid selector.

};
#pragma pack(pop)

struct gdt64_descriptor_s {
	uint16_t limit_low;           // The lower 16 bits of the limit.
	uint16_t base_low;            // The lower 16 bits of the base.
	uint8_t  base_middle;         // The next 8 bits of the base.
	gdt_entry_access_t  access;              // Access flags, determine what ring this segment can be used in.
	uint8_t  granularity;
	uint8_t  base_high;
};

void init_idt(idtr_s* pIDT, uint32_t base, uint32_t limit, uint16_t flags);
void init_gdt(gdtr_s* pGDT, uint32_t base, uint32_t limit, uint16_t flags);
void print_gdt_descriptor(uint64_t gdtBase, gdt64_descriptor_s* pDesc);