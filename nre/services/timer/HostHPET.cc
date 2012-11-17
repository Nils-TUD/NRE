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

void HostHPET::HPETTimer::init(HostTimerDevice &dev, cpu_t cpu) {
    // Prefer MSIs. No sharing, no routing problems, always edge triggered.
    if(!(_reg->config & LEG_RT_CNF) && (_reg->config & FSB_INT_DEL_CAP)) {
        uint16_t rid = get_rid(0, _no);
        LOG(TIMER, "TIMER: HPET comparator " << _no << " RID " << fmt(rid, "x") << "\n");

        // in this case we have to pass the MMIO space cap to NOVA instead of the MMConfig space cap
        // because the HPET is no PCI device, but does only use a PCI requester ID outside of
        // the regular PCI bus range.
        HostHPET &hpet = static_cast<HostHPET&>(dev);
        _gsi = new Gsi(reinterpret_cast<void*>(hpet._mem.virt()), cpu);

        LOG(TIMER, "TIMER: Timer " << _no << " -> GSI " << _gsi->gsi() << " CPU " << cpu
                                   << " (" << fmt(_gsi->msi_addr(), "#x") << ":"
                                   << fmt(_gsi->msi_value(), "#x") << "\n");

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
            throw Exception(E_CAPACITY, "No IRQs left");

        uint irq = Math::bit_scan_reverse(possible_irqs);
        _assigned_irqs |= (1U << irq);

        _gsi = new Gsi(irq, cpu);
        _ack = (irq < 16) ? 0 : (1U << _no);

        LOG(TIMER, "TIMER: Timer " << _no << " -> IRQ " << irq
                                   << " (assigned " << fmt(_assigned_irqs, "#x")
                                   << " ack " << fmt(_ack, "#x") << ").\n");

        _reg->config &= ~(0x1F << 9) | INT_TYPE_CNF;
        _reg->config |= (irq << 9) | ((irq < 16) ? 0 /* Edge */ : INT_TYPE_CNF /* Level */);
    }
}

HostHPET::HostHPET(bool force_legacy)
    : _addr(get_address()),
      _mem(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::RW, _addr),
      _reg(reinterpret_cast<HostHpetRegister*>(_mem.virt())), _usable_timers(0) {
    bool legacy_only = force_legacy;
    LOG(TIMER_DETAIL, "TIMER: HPET at " << fmt(_addr, "p") << " -> " << fmt(_mem.virt(), "p") << "\n");

    // Check for old AMD HPETs and go home. :)
    uint8_t rev = _reg->cap & 0xFF;
    uint16_t vendor = _reg->cap >> 16;
    LOG(TIMER_DETAIL, "TIMER: HPET" << " vendor " << fmt(vendor, "#0x", 4)
                                    << " revision " << fmt(rev, "#0x", 2) << ":"
                                    << ((_reg->cap & LEG_RT_CAP) ? " LEGACY" : "")
                                    << ((_reg->cap & BIT64_CAP) ? " 64BIT" : " 32BIT") << "\n");
    switch(vendor) {
        case 0x8086: // Intel
            // Everything OK.
            break;
        case 0x4353: // AMD
            if(rev < 0x10) {
                LOG(TIMER, "TIMER: It's one of those old broken AMD HPETs. Use legacy mode.\n");
                legacy_only = true;
            }
            break;
        default:
            // Before you blindly enable features for other HPETs, check
            // Linux and FreeBSD source for quirks!
            LOG(TIMER, "TIMER: Unknown HPET vendor ID. We only trust legacy mode.\n");
            legacy_only = true;
            break;
    }

    if(legacy_only && !(_reg->cap & LEG_RT_CAP)) {
        // XXX Implement PIT mode
        throw Exception(E_NOT_FOUND, "We want legacy mode, but the timer doesn't support it");
    }

    // Figure out how many HPET timers are usable
    uint32_t cap = _reg->cap, config = _reg->config, period = _reg->period;
    LOG(TIMER_DETAIL, "TIMER: HPET:" << " cap " << fmt(cap, "#x")
                                     << " config " << fmt(config, "#x")
                                     << " period " << period << "\n");
    uint timers = ((_reg->cap >> 8) & 0x1F) + 1;

    uint32_t combined_irqs = ~0U;
    for(uint i = 0; i < timers; i++) {
        LOG(TIMER_DETAIL, "TIMER: HPET Timer[" << i << "]:"
                          << " config " << fmt(_reg->timer[i].config, "#x")
                          << " int " << fmt(_reg->timer[i].int_route, "#x") << "\n");
        if((_reg->timer[i].config | _reg->timer[i].int_route) == 0) {
            LOG(TIMER, "TIMER:\tTimer[" << i << "] seems bogus. Ignore.\n");
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
        LOG(TIMER_DETAIL, "TIMER: Reducing usable timers from " << _usable_timers << " to " << irqs
                          << ". Combined IRQ mask is " << fmt(combined_irqs, "#x") << "\n");
        _usable_timers = irqs;
    }

    // TODO Can this happen?
    if(_usable_timers == 0)
        throw Exception(E_NOT_FOUND, "No suitable HPET timer");

    if(legacy_only) {
        LOG(TIMER, "TIMER: Use one timer in legacy mode.\n");
        _usable_timers = 1;
    }
    else
        LOG(TIMER, "TIMER: Found " << _usable_timers << " usable timers.\n");

    if(_usable_timers > CPU::count()) {
        _usable_timers = CPU::count();
        LOG(TIMER, "TIMER: More timers than CPUs. " << " Use only " << _usable_timers << " timers.\n");
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
    LOG(TIMER, "TIMER: HPET ticks with " << _freq << " HZ.\n");
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
uint16_t HostHPET::get_rid(uint8_t block, uint comparator) {
    Connection con("acpi");
    ACPISession sess(con);
    ACPI::RSDT *addr = sess.find_table("DMAR");
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
        while(s.has_next() and ((s = s.next()), true));
    }
    while(e.has_next() and ((e = e.next()), true));

    // When we get here, either we haven't found a single RID or only
    // one. For the latter case, we assume it's the right one.
    return first_rid_found;
}

uintptr_t HostHPET::get_address() {
    Connection con("acpi");
    ACPISession sess(con);
    ACPI::RSDT *addr = sess.find_table("HPET");
    if(!addr)
        throw Exception(E_NOT_FOUND, "Unable to find HPET in ACPI tables");

    struct HpetAcpiTable {
        char res[40];
        unsigned char gas[4];
        uint32_t address[2];
    };
    HpetAcpiTable *table = reinterpret_cast<HpetAcpiTable*>(addr);
    if(table->gas[0])
        throw Exception(E_NOT_FOUND, "HPET access must be MMIO");
    if(table->address[1])
        throw Exception(E_NOT_FOUND, "HPET must be below 4G");
    return table->address[0];
}
