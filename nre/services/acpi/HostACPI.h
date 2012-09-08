/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009-2010, Bernhard Kauer <bk@vmmon.org>
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

#pragma once

#include <arch/Types.h>
#include <services/ACPI.h>
#include <Compiler.h>

class HostACPI {
	enum {
		BIOS_MEM_ADDR	= 0xe0000,
		BIOS_MEM_SIZE	= 0x20000,
		BIOS_ADDR		= 0x0,
		BIOS_SIZE		= 0x1000,
		BIOS_EBDA_OFF	= 0x40E,
		BIOS_EBDA_SIZE	= 1024
	};

	/* root system descriptor pointer */
	struct RSDP {
		uint32_t signature[2];
		uint8_t checksum;
		char oemId[6];
		uint8_t revision;
		uint32_t rsdtAddr;
		/* since 2.0 */
		uint32_t length;
		uint64_t xsdtAddr;
		uint8_t xchecksum;
	} PACKED;

	/* APIC Structure (5.2.11.4) */
	struct APIC {
		enum Type {
			LAPIC = 0, IOAPIC = 1, INTR = 2,
		};
		uint8_t type;
		uint8_t length;
	} PACKED;

	/* Interrupt Source Override (5.2.11.8) */
	struct APICIntr : public APIC {
		uint8_t bus;
		uint8_t irq;
		uint32_t gsi;
		uint16_t flags;
	} PACKED;

	/* Multiple APIC Description Table */
	struct MADT : public nre::ACPI::RSDT {
		uint32_t apic_addr;
		uint32_t flags;
		APIC apic[];
	} PACKED;

public:
	explicit HostACPI();

	const nre::DataSpace &mem() const {
		return *_ds;
	}
	uintptr_t find(const char *name,uint instance,size_t &length);
	uint irq_to_gsi(uint irq);

private:
	static char checksum(char *table,unsigned count) {
		char res = 0;
		while(count--)
			res += table[count];
		return res;
	}
	static RSDP *get_rsdp();

private:
	size_t _count;
	nre::ACPI::RSDT **_tables;
	RSDP *_rsdp;
	nre::DataSpace *_ds;
};
