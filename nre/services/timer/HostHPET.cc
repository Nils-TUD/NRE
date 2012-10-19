/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2011, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Bernhard Kauer <bk@vmmon.org>
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

#include <services/ACPI.h>
#include <services/PCIConfig.h>
#include <stream/Serial.h>
#include <util/DmarTableParser.h>
#include <util/Math.h>
#include <Logging.h>
#include <Hip.h>

#include "HostHPET.h"

using namespace nre;

uint HostHPET::HPETTimer::_assigned_irqs = 0;

void HostHPET::HPETTimer::init(cpu_t cpu) {
	// Prefer MSIs. No sharing, no routing problems, always edge triggered.
	if(!(_reg->config & LEG_RT_CNF) && (_reg->config & FSB_INT_DEL_CAP)) {
		uint16_t rid = get_rid(0,_no);
		LOG(Logging::TIMER,Serial::get().writef("TIMER: HPET comparator %u RID %x\n",_no,rid));

		Connection con("pcicfg");
		PCIConfigSession pcicfg(con);
		uintptr_t phys_addr = pcicfg.addr(rid,0);
		DataSpace hpetds(ExecEnv::PAGE_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::R,phys_addr);
		_gsi = new Gsi(reinterpret_cast<void*>(hpetds.virt()),cpu);

		LOG(Logging::TIMER,Serial::get().writef("TIMER: Timer %u -> GSI %u CPU %u (%#Lx:%#lx)\n",
				_no,_gsi->gsi(),cpu,_gsi->msi_addr(),_gsi->msi_value()));

		_ack = 0;
		_reg->msi[0] = _gsi->msi_value();
		_reg->msi[1] = _gsi->msi_addr();
		_reg->config |= FSB_INT_EN_CNF;
	}
	else {
		// If legacy is enabled, only allow IRQ2
		uint32_t allowed_irqs = (_reg->config & LEG_RT_CNF) ? (1U << 2) : _reg->int_route;
		uint32_t possible_irqs = ~_assigned_irqs & allowed_irqs;
		if(possible_irqs == 0)
			throw Exception(E_CAPACITY,"No IRQs left");

		uint irq = Math::bit_scan_reverse(possible_irqs);
		_assigned_irqs |= (1U << irq);

		_gsi = new Gsi(irq,cpu);
		_ack = (irq < 16) ? 0 : (1U << _no);

		LOG(Logging::TIMER,Serial::get().writef("TIMER: Timer %u -> IRQ %u (assigned %#x ack %#x).\n",
				_no,irq,_assigned_irqs,_ack));

		_reg->config &= ~(0x1F << 9) | INT_TYPE_CNF;
		_reg->config |= (irq << 9) | ((irq < 16) ? 0 /* Edge */: INT_TYPE_CNF /* Level */);
	}
}

HostHPET::HostHPET(bool force_legacy)
		: _addr(get_address()),
		  _mem(ExecEnv::PAGE_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::RW,_addr),
		  _reg(reinterpret_cast<HostHpetRegister*>(_mem.virt())), _usable_timers(0) {
	bool legacy_only = force_legacy;
	LOG(Logging::TIMER_DETAIL,Serial::get().writef("TIMER: HPET at %p -> %p\n",
			reinterpret_cast<void*>(_addr),reinterpret_cast<void*>(_mem.virt())));

	// Check for old AMD HPETs and go home. :)
	uint8_t rev = _reg->cap & 0xFF;
	uint16_t vendor = _reg->cap >> 16;
	LOG(Logging::TIMER_DETAIL,Serial::get().writef("TIMER: HPET vendor %#04x revision %#02x:%s%s\n",
			vendor,rev,(_reg->cap & LEG_RT_CAP) ? " LEGACY" : "",
	        (_reg->cap & BIT64_CAP) ? " 64BIT" : " 32BIT"));
	switch(vendor) {
		case 0x8086: // Intel
			// Everything OK.
			break;
		case 0x4353: // AMD
			if(rev < 0x10) {
				LOG(Logging::TIMER,Serial::get().writef(
						"TIMER: It's one of those old broken AMD HPETs. Use legacy mode.\n"));
				legacy_only = true;
			}
			break;
		default:
			// Before you blindly enable features for other HPETs, check
			// Linux and FreeBSD source for quirks!
			LOG(Logging::TIMER,Serial::get().writef(
					"TIMER: Unknown HPET vendor ID. We only trust legacy mode.\n"));
			legacy_only = true;
			break;
	}

	if(legacy_only && !(_reg->cap & LEG_RT_CAP)) {
		// XXX Implement PIT mode
		throw Exception(E_NOT_FOUND,"We want legacy mode, but the timer doesn't support it");
	}

	// Figure out how many HPET timers are usable
	LOG(Logging::TIMER_DETAIL,Serial::get().writef("TIMER: HPET: cap %#x config %#x period %d\n",
			_reg->cap,_reg->config,_reg->period));
	uint timers = ((_reg->cap >> 8) & 0x1F) + 1;

	uint32_t combined_irqs = ~0U;
	for(uint i = 0; i < timers; i++) {
		LOG(Logging::TIMER_DETAIL,Serial::get().writef("TIMER: HPET Timer[%d]: config %#x int %#x\n",
				i,_reg->timer[i].config,_reg->timer[i].int_route));
		if((_reg->timer[i].config | _reg->timer[i].int_route) == 0) {
			LOG(Logging::TIMER,Serial::get().writef("TIMER:\tTimer[%d] seems bogus. Ignore.\n",i));
			continue;
		}

		if((_reg->timer[i].config & FSB_INT_DEL_CAP) == 0) {
			// Comparator is not MSI capable.
			combined_irqs &= _reg->timer[i].int_route;
		}

		_timer[_usable_timers]._no = i;
		_timer[_usable_timers]._reg = &_reg->timer[i];
		_usable_timers++;
	}

	// Reduce the number of comparators to ones we can assign individual IRQs.
	// TODO This overcompensates and is suboptimal. We need backtracking search. It's a bin packing problem.
	uint irqs = Math::popcount(combined_irqs);
	if(irqs < _usable_timers) {
		LOG(Logging::TIMER_DETAIL,Serial::get().writef(
				"TIMER: Reducing usable timers from %u to %u. Combined IRQ mask is %#x\n",
		        _usable_timers,irqs,combined_irqs));
		_usable_timers = irqs;
	}

	// TODO Can this happen?
	if(_usable_timers == 0)
		throw Exception(E_NOT_FOUND,"No suitable HPET timer");

	if(legacy_only) {
		LOG(Logging::TIMER,Serial::get().writef("TIMER: Use one timer in legacy mode.\n"));
		_usable_timers = 1;
	}
	else
		LOG(Logging::TIMER,Serial::get().writef("TIMER: Found %u usable timers.\n",_usable_timers));

	if(_usable_timers > CPU::count()) {
		_usable_timers = CPU::count();
		LOG(Logging::TIMER,Serial::get().writef(
				"TIMER: More timers than CPUs. (Good!) Use only %u timers.\n",_usable_timers));
	}

	for(uint i = 0; i < _usable_timers; i++) {
		// Interrupts will be disabled now. Will be enabled when the corresponding per_cpu thread comes up.
		_timer[i]._reg->config |= MODE32_CNF;
		_timer[i]._reg->config &= ~(INT_ENB_CNF | TYPE_CNF);
		_timer[i]._reg->comp64 = 0;
	}

	// Disable counting and IRQs. Program legacy mode as requested.
	_reg->isr = ~0U;
	_reg->config &= ~(ENABLE_CNF | LEG_RT_CNF);
	_reg->main = 0ULL;
	_reg->config |= (legacy_only ? LEG_RT_CNF : 0);

	// HPET configuration
	_freq = 1000000000000000ULL / _reg->period;
	LOG(Logging::TIMER,Serial::get().writef("TIMER: HPET ticks with %Lu HZ.\n",_freq));
}

/**
 * Try to find out HPET routing ID. Returns 0 on failure.
 *
 * HPET routing IDs are stored in the DMAR table and we need them
 * for interrupt remapping to work. This method is complicated by
 * weird BIOSes that give each comparator its own RID. Our heuristic
 * is to check, whether we see multiple device scope entries per
 * ID. If this is the case, we assume that each comparator has a
 * different RID.
 */
uint16_t HostHPET::get_rid(uint8_t block,uint comparator) {
	Connection con("acpi");
	ACPISession sess(con);
	ACPI::RSDT *addr = sess.find_table(String("DMAR"));
	if(!addr)
		return 0;

	DmarTableParser p(reinterpret_cast<char*>(addr));
	DmarTableParser::Element e = p.get_element();

	uint16_t first_rid_found = 0;
	do {
		if(e.type() != DmarTableParser::DHRD)
			continue;

		DmarTableParser::Dhrd dhrd = e.get_dhrd();
		if(!dhrd.has_scopes())
			continue;

		DmarTableParser::DeviceScope s = dhrd.get_scope();
		do {
			if(s.type() != DmarTableParser::MSI_CAPABLE_HPET)
				continue;

			// We assume the enumaration IDs correspond to timer blocks.
			// XXX Is this true? We haven't seen an HPET with multiple blocks yet.
			if(s.id() != block)
				continue;

			if(first_rid_found == 0)
				first_rid_found = s.rid();

			// Return the RID for the right comparator.
			if(comparator-- == 0)
				return s.rid();
		}
		while(s.has_next() and ((s = s.next()),true));
	}
	while(e.has_next() and ((e = e.next()),true));

	// When we get here, either we haven't found a single RID or only
	// one. For the latter case, we assume it's the right one.
	return first_rid_found;
}

uintptr_t HostHPET::get_address() {
	Connection con("acpi");
	ACPISession sess(con);
	ACPI::RSDT *addr = sess.find_table(String("HPET"));
	if(!addr)
		throw Exception(E_NOT_FOUND,"Unable to find HPET in ACPI tables");

	struct HpetAcpiTable {
		char res[40];
		unsigned char gas[4];
		uint32_t address[2];
	};
	HpetAcpiTable *table = reinterpret_cast<HpetAcpiTable*>(addr);
	if(table->gas[0])
		throw Exception(E_NOT_FOUND,"HPET access must be MMIO");
	if(table->address[1])
		throw Exception(E_NOT_FOUND,"HPET must be below 4G");
	return table->address[0];
}
