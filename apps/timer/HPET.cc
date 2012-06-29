/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "HPET.h"

#if 0
void HPET::HPET(bool force_legacy) : _mem() {
	// Find and map HPET
	bool legacy_only = force_legacy;
	unsigned long hpet_addr = get_hpet_address(_mb.bus_acpi);
	if(hpet_addr == 0) {
		Logging::printf("TIMER: No HPET found.\n");
		return false;
	}

	MessageHostOp msg1(MessageHostOp::OP_ALLOC_IOMEM,hpet_addr,1024);
	if(!_mb.bus_hostop.send(msg1) || !msg1.ptr) {
		Logging::printf("TIMER: %s failed to allocate iomem %lx+0x400\n",__PRETTY_FUNCTION__,
		        hpet_addr);
		return false;
	}

	_reg = reinterpret_cast<HostHpetRegister *>(msg1.ptr);
	if(_verbose)
		Logging::printf("TIMER: HPET at %08lx -> %p.\n",hpet_addr,_reg);

	// Check for old AMD HPETs and go home. :)
	uint8 hpet_rev = _reg->cap & 0xFF;
	uint16 hpet_vendor = _reg->cap >> 16;
	Logging::printf("TIMER: HPET vendor %04x revision %02x:%s%s\n",hpet_vendor,hpet_rev,
	        (_reg->cap & LEG_RT_CAP) ? " LEGACY" : "",
	        (_reg->cap & BIT64_CAP) ? " 64BIT" : " 32BIT");
	switch(hpet_vendor) {
		case 0x8086: // Intel
			// Everything OK.
			break;
		case 0x4353: // AMD
			if(hpet_rev < 0x10) {
				Logging::printf(
				        "TIMER: It's one of those old broken AMD HPETs. Use legacy mode.\n");
				legacy_only = true;
			}
			break;
		default:
			// Before you blindly enable features for other HPETs, check
			// Linux and FreeBSD source for quirks!
			Logging::printf("TIMER: Unknown HPET vendor ID. We only trust legacy mode.\n");
			legacy_only = true;
	}

	if(legacy_only and not (_reg->cap & LEG_RT_CAP)) {
		// XXX Implement PIT mode
		Logging::printf("TIMER: We want legacy mode, but the timer doesn't support it.\n");
		return false;
	}

	// Figure out how many HPET timers are usable
	if(_verbose)
		Logging::printf("TIMER: HPET: cap %x config %x period %d\n",_reg->cap,_reg->config,
		        _reg->period);
	unsigned timers = ((_reg->cap >> 8) & 0x1F) + 1;

	_usable_timers = 0;

	uint32 combined_irqs = ~0U;
	for(unsigned i = 0; i < timers; i++) {
		if(_verbose)
			Logging::printf("TIMER: HPET Timer[%d]: config %x int %x\n",i,_reg->timer[i].config,
			        _reg->timer[i].int_route);
		if((_reg->timer[i].config | _reg->timer[i].int_route) == 0) {
			Logging::printf("TIMER:\tTimer[%d] seems bogus. Ignore.\n",i);
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

	// Reduce the number of comparators to ones we can assign
	// individual IRQs.
	// XXX This overcompensates and is suboptimal. We need
	// backtracking search. It's a bin packing problem.
	unsigned irqs = Cpu::popcount(combined_irqs);
	if(irqs < _usable_timers) {
		Logging::printf("TIMER: Reducing usable timers from %u to %u. Combined IRQ mask is %x\n",
		        _usable_timers,irqs,combined_irqs);
		_usable_timers = irqs;
	}

	if(_usable_timers == 0) {
		// XXX Can this happen?
		Logging::printf("TIMER: No suitable HPET timer.\n");
		return false;

	}

	if(legacy_only) {
		Logging::printf("TIMER: Use one timer in legacy mode.\n");
		_usable_timers = 1;
	}
	else
		Logging::printf("TIMER: Found %u usable timers.\n",_usable_timers);

	if(_usable_timers > _mb.hip()->cpu_count()) {
		_usable_timers = _mb.hip()->cpu_count();
		Logging::printf("TIMER: More timers than CPUs. (Good!) Use only %u timers.\n",
		        _usable_timers);
	}

	for(unsigned i = 0; i < _usable_timers; i++) {
		// Interrupts will be disabled now. Will be enabled when the
		// corresponding per_cpu thread comes up.
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

	uint64 freq = 1000000000000000ULL;
	Math::div64(freq,_reg->period);
	_timer_freq = freq;
	Logging::printf("TIMER: HPET ticks with %u HZ.\n",_timer_freq);

	return true;
}

#endif
