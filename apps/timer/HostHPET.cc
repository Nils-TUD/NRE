/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <dev/ACPI.h>
#include <stream/Serial.h>
#include <util/Math.h>
#include <Hip.h>

#include "HostHPET.h"

using namespace nul;

bool HostHPET::_verbose = true;

HostHPET::HostHPET(bool force_legacy)
		: _addr(get_address()),
		  _mem(ExecEnv::PAGE_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::RW,_addr),
		  _reg(reinterpret_cast<HostHpetRegister*>(_mem.virt())), _usable_timers(0) {
	bool legacy_only = force_legacy;
	if(_verbose)
		Serial::get().writef("TIMER: HPET at %p -> %p\n",_addr,_mem.virt());

	// Check for old AMD HPETs and go home. :)
	uint8_t rev = _reg->cap & 0xFF;
	uint16_t vendor = _reg->cap >> 16;
	Serial::get().writef("TIMER: HPET vendor %#04x revision %#02x:%s%s\n",vendor,rev,
	        (_reg->cap & LEG_RT_CAP) ? " LEGACY" : "",
	        (_reg->cap & BIT64_CAP) ? " 64BIT" : " 32BIT");
	switch(vendor) {
		case 0x8086: // Intel
			// Everything OK.
			break;
		case 0x4353: // AMD
			if(rev < 0x10) {
				Serial::get().writef("TIMER: It's one of those old broken AMD HPETs. Use legacy mode.\n");
				legacy_only = true;
			}
			break;
		default:
			// Before you blindly enable features for other HPETs, check
			// Linux and FreeBSD source for quirks!
			Serial::get().writef("TIMER: Unknown HPET vendor ID. We only trust legacy mode.\n");
			legacy_only = true;
			break;
	}

	if(legacy_only && !(_reg->cap & LEG_RT_CAP)) {
		// XXX Implement PIT mode
		throw Exception(E_NOT_FOUND,"We want legacy mode, but the timer doesn't support it");
	}

	// Figure out how many HPET timers are usable
	if(_verbose)
		Serial::get().writef("TIMER: HPET: cap %#x config %#x period %d\n",_reg->cap,_reg->config,_reg->period);
	uint timers = ((_reg->cap >> 8) & 0x1F) + 1;

	uint32_t combined_irqs = ~0U;
	for(uint i = 0; i < timers; i++) {
		if(_verbose) {
			Serial::get().writef("TIMER: HPET Timer[%d]: config %#x int %#x\n",
					i,_reg->timer[i].config,_reg->timer[i].int_route);
		}
		if((_reg->timer[i].config | _reg->timer[i].int_route) == 0) {
			Serial::get().writef("TIMER:\tTimer[%d] seems bogus. Ignore.\n",i);
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
		Serial::get().writef("TIMER: Reducing usable timers from %u to %u. Combined IRQ mask is %#x\n",
		        _usable_timers,irqs,combined_irqs);
		_usable_timers = irqs;
	}

    // TODO Can this happen?
	if(_usable_timers == 0)
		throw Exception(E_NOT_FOUND,"No suitable HPET timer");

	if(legacy_only) {
		Serial::get().writef("TIMER: Use one timer in legacy mode.\n");
		_usable_timers = 1;
	}
	else
		Serial::get().writef("TIMER: Found %u usable timers.\n",_usable_timers);

	if(_usable_timers > CPU::count()) {
		_usable_timers = CPU::count();
		Serial::get().writef("TIMER: More timers than CPUs. (Good!) Use only %u timers.\n",_usable_timers);
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
	Serial::get().writef("TIMER: HPET ticks with %Lu HZ.\n",_freq);
}

uintptr_t HostHPET::get_address() {
	Connection con("acpi");
	ACPISession sess(con);
	uintptr_t addr = sess.find_table(String("HPET"));
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
