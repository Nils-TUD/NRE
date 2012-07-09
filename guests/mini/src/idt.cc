#include "idt.h"

IDT::Entry IDT::idt[256];
extern "C" void defaultEx(void);
extern "C" void defaultInt(void);

void IDT::init() {
	Ptr idtPtr;
	idtPtr.address = (uintptr_t)idt;
	idtPtr.size = sizeof(idt) - 1;

	for(size_t i = 0; i < 0x20; i++)
		set(i,defaultEx);
	for(size_t i = 0x20; i < ARRAY_SIZE(idt); i++)
		set(i,defaultInt);

	__asm__ volatile ("lidt %0" : : "m" (idtPtr));
}

void IDT::set(size_t irq,callback_t cb) {
	assert(irq < ARRAY_SIZE(idt));
	idt[irq].fix = 0xE00;
	idt[irq].dpl = 0;
	idt[irq].present = irq != INTEL_RES1 && irq != INTEL_RES2;
	idt[irq].selector = 0x8;
	idt[irq].offsetHigh = ((uintptr_t)cb >> 16) & 0xFFFF;
	idt[irq].offsetLow = (uintptr_t)cb & 0xFFFF;
}

