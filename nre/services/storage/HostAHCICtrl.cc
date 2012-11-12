/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2008, Bernhard Kauer <bk@vmmon.org>
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

#include <services/PCIConfig.h>
#include <services/ACPI.h>
#include <Logging.h>

#include "HostAHCICtrl.h"

using namespace nre;

HostAHCICtrl::HostAHCICtrl(uint id, PCI &pci, PCI::bdf_type bdf, Gsi *gsi, bool dmar)
    : Controller(id), _gsi(gsi), _bdf(bdf), _regs_ds(), _regs_high_ds(), _regs(),
      _regs_high(0), _portcount(0), _ports() {
    assert(!(~pci.conf_read(_bdf, 1) & 6) && "we need mem-decode and busmaster dma");
    PCI::value_type bar = pci.conf_read(_bdf, 9);
    assert(!(bar & 7) && "we need a 32bit memory bar");

    _regs_ds = new DataSpace(0x1000, DataSpaceDesc::LOCKED, DataSpaceDesc::RW, bar);
    _regs = reinterpret_cast<Register*>(_regs_ds->virt() + (bar & 0xFFF));

    // map the high ports
    if(_regs->pi >> 30) {
        _regs_high_ds = new DataSpace(0x1000, DataSpaceDesc::LOCKED, DataSpaceDesc::RW, bar + 0x1000);
        _regs_high = reinterpret_cast<HostAHCIDevice::Register*>(
            _regs_high_ds->virt() + (bar & 0xFE0));
    }

    // enable AHCI
    _regs->ghc |= 0x80000000;
    LOG(Logging::STORAGE, Serial::get().writef(
            "AHCI: cap %#x cap2 %#x global %#x ports %#x version %#x bohc %#x\n",
            _regs->cap, _regs->cap2, _regs->ghc, _regs->pi, _regs->vs, _regs->bohc));
    assert(!_regs->bohc);

    // create ports
    memset(_ports, 0, sizeof(_ports));
    for(uint i = 0; i < 30; i++)
        create_ahci_port(i, _regs->ports + i, dmar);
    for(uint i = 30; _regs_high && i < 32; i++)
        create_ahci_port(i, _regs_high + (i - 30), dmar);

    // clear pending irqs
    _regs->is = _regs->pi;
    // enable IRQs
    _regs->ghc |= 2;

    // start the gsi thread
    char name[32];
    OStringStream os(name, sizeof(name));
    os << "ahci-gsi-" << _gsi->gsi();
    GlobalThread *gt = GlobalThread::create(gsi_thread, CPU::current().log_id(), name);
    gt->set_tls<HostAHCICtrl*>(Thread::TLS_PARAM, this);
    gt->start();
}

void HostAHCICtrl::create_ahci_port(uint nr, HostAHCIDevice::Register *portreg, bool dmar) {
    // port not implemented
    if(!(_regs->pi & (1 << nr)))
        return;

    // check what kind of drive we have, if any
    uint32_t sig = HostAHCIDevice::get_signature(portreg);
    if(sig != HostAHCIDevice::SATA_SIG_NONE) {
        try {
            _ports[nr] = new HostAHCIDevice(portreg, _id * Storage::MAX_DRIVES + _portcount,
                                            ((_regs->cap >> 8) & 0x1f) + 1, dmar);
            _ports[nr]->determine_capacity();
            LOG(Logging::STORAGE, _ports[nr]->print());
            _portcount++;
        }
        catch(const Exception &e) {
            LOG(Logging::STORAGE, Serial::get().writef(
                    "Unable to create AHCI device for port %u: %s\n", nr, e.msg()));
            _ports[nr] = 0;
        }
    }
}

void HostAHCICtrl::gsi_thread(void*) {
    HostAHCICtrl *ha = Thread::current()->get_tls<HostAHCICtrl*>(Thread::TLS_PARAM);
    while(1) {
        ha->_gsi->down();

        uint32_t is = ha->_regs->is;
        uint32_t oldis = is;
        while(is) {
            uint32_t port = Math::bit_scan_forward(is);
            if(ha->_ports[port])
                ha->_ports[port]->irq();
            is &= ~(1 << port);
        }
        ha->_regs->is = oldis;
    }
}
