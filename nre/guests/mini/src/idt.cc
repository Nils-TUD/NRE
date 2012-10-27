/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include "idt.h"

IDT::Entry IDT::idt[256];
IDT::callback_t IDT::callbacks[256];

extern "C" void entry_ex(void);
extern "C" void entry_int(void);

extern "C" void entry_0(void);
extern "C" void entry_1(void);
extern "C" void entry_2(void);
extern "C" void entry_3(void);
extern "C" void entry_4(void);
extern "C" void entry_5(void);
extern "C" void entry_6(void);
extern "C" void entry_7(void);
extern "C" void entry_8(void);
extern "C" void entry_9(void);
extern "C" void entry_10(void);
extern "C" void entry_11(void);
extern "C" void entry_12(void);
extern "C" void entry_13(void);
extern "C" void entry_14(void);
extern "C" void entry_15(void);
extern "C" void entry_16(void);
extern "C" void entry_17(void);
extern "C" void entry_18(void);
extern "C" void entry_19(void);
extern "C" void entry_20(void);
extern "C" void entry_21(void);
extern "C" void entry_22(void);
extern "C" void entry_23(void);
extern "C" void entry_24(void);
extern "C" void entry_25(void);
extern "C" void entry_26(void);
extern "C" void entry_27(void);
extern "C" void entry_28(void);
extern "C" void entry_29(void);
extern "C" void entry_30(void);
extern "C" void entry_31(void);
extern "C" void entry_32(void);
extern "C" void entry_33(void);
extern "C" void entry_34(void);
extern "C" void entry_35(void);
extern "C" void entry_36(void);
extern "C" void entry_37(void);
extern "C" void entry_38(void);
extern "C" void entry_39(void);
extern "C" void entry_40(void);
extern "C" void entry_41(void);
extern "C" void entry_42(void);
extern "C" void entry_43(void);
extern "C" void entry_44(void);
extern "C" void entry_45(void);
extern "C" void entry_46(void);
extern "C" void entry_47(void);

void IDT::dummy() {
}

void IDT::init() {
	Ptr idtPtr;
	idtPtr.address = (uintptr_t)idt;
	idtPtr.size = sizeof(idt) - 1;

	/* exceptions */
	setup(0, entry_0);
	setup(1, entry_1);
	setup(2, entry_2);
	setup(3, entry_3);
	setup(4, entry_4);
	setup(5, entry_5);
	setup(6, entry_6);
	setup(7, entry_7);
	setup(8, entry_8);
	setup(9, entry_9);
	setup(10, entry_10);
	setup(11, entry_11);
	setup(12, entry_12);
	setup(13, entry_13);
	setup(14, entry_14);
	setup(15, entry_15);
	setup(16, entry_16);
	setup(17, entry_17);
	setup(18, entry_18);
	setup(19, entry_19);
	setup(20, entry_20);
	setup(21, entry_21);
	setup(22, entry_22);
	setup(23, entry_23);
	setup(24, entry_24);
	setup(25, entry_25);
	setup(26, entry_26);
	setup(27, entry_27);
	setup(28, entry_28);
	setup(29, entry_29);
	setup(30, entry_30);
	setup(31, entry_31);
	setup(32, entry_32);

	/* hardware-interrupts */
	setup(33, entry_33);
	setup(34, entry_34);
	setup(35, entry_35);
	setup(36, entry_36);
	setup(37, entry_37);
	setup(38, entry_38);
	setup(39, entry_39);
	setup(40, entry_40);
	setup(41, entry_41);
	setup(42, entry_42);
	setup(43, entry_43);
	setup(44, entry_44);
	setup(45, entry_45);
	setup(46, entry_46);
	setup(47, entry_47);

	for(size_t i = 0; i < ARRAY_SIZE(idt); i++)
		set(i, dummy);

	__asm__ volatile ("lidt %0" : : "m" (idtPtr));
}

void IDT::setup(size_t irq, callback_t cb) {
	assert(irq < ARRAY_SIZE(idt));
	idt[irq].fix = 0xE00;
	idt[irq].dpl = 0;
	idt[irq].present = irq != INTEL_RES1 && irq != INTEL_RES2;
	idt[irq].selector = 0x8;
	idt[irq].offsetHigh = ((uintptr_t)cb >> 16) & 0xFFFF;
	idt[irq].offsetLow = (uintptr_t)cb & 0xFFFF;
}

void IDT::set(size_t irq, callback_t cb) {
	assert(irq < ARRAY_SIZE(idt));
	callbacks[irq] = cb;
}

