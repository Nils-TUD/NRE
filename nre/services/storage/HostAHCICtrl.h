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

#pragma once

#include <kobj/Gsi.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>
#include <util/PCI.h>
#include <Assert.h>
#include <CPU.h>

#include "HostAHCIDevice.h"
#include "Controller.h"

/**
 * A simple driver for AHCI.
 *
 * State: testing
 * Features: Ports
 */
class HostAHCICtrl : public Controller {
    /**
     * The register set of an AHCI controller.
     */
    struct Register {
        union {
            struct {
                volatile uint32_t cap;
                volatile uint32_t ghc;
                volatile uint32_t is;
                volatile uint32_t pi;
                volatile uint32_t vs;
                volatile uint32_t ccc_ctl;
                volatile uint32_t ccc_ports;
                volatile uint32_t em_loc;
                volatile uint32_t em_ctl;
                volatile uint32_t cap2;
                volatile uint32_t bohc;
            };
            volatile uint32_t generic[0x100 >> 2];
        };
        HostAHCIDevice::Register ports[32];
    };

public:
    explicit HostAHCICtrl(uint id, nre::PCI &pci, nre::PCI::bdf_type bdf, nre::Gsi *gsi, bool dmar);
    virtual ~HostAHCICtrl() {
        delete _gsi;
        delete _regs_ds;
        delete _regs_high_ds;
    }

    virtual bool exists(size_t drive) const {
        return idx(drive) < ARRAY_SIZE(_ports) && _ports[idx(drive)] != nullptr;
    }
    virtual size_t drive_count() const {
        return _portcount;
    }
    virtual void get_params(size_t drive, nre::Storage::Parameter *params) const {
        assert(_ports[idx(drive)]);
        _ports[idx(drive)]->get_params(params);
    }

    virtual void flush(size_t drive, producer_type *prod, tag_type tag) {
        assert(_ports[idx(drive)]);
        _ports[idx(drive)]->flush(prod, tag);
    }
    virtual void read(size_t drive, producer_type *prod, tag_type tag, const nre::DataSpace &ds,
                      sector_type sector, const dma_type &dma) {
        assert(_ports[idx(drive)]);
        _ports[idx(drive)]->readwrite(prod, tag, ds, sector, dma, false);
    }
    virtual void write(size_t drive, producer_type *prod, tag_type tag, const nre::DataSpace &ds,
                       sector_type sector, const dma_type &dma) {
        assert(_ports[idx(drive)]);
        _ports[idx(drive)]->readwrite(prod, tag, ds, sector, dma, true);
    }

private:
    static size_t idx(size_t drive) {
        return drive % nre::Storage::MAX_DRIVES;
    }
    void create_ahci_port(uint nr, HostAHCIDevice::Register *portreg, bool dmar);
    static void gsi_thread(void*);

    nre::Gsi *_gsi;
    nre::PCI::bdf_type _bdf;
    uint _hostirq;
    nre::DataSpace *_regs_ds;
    nre::DataSpace *_regs_high_ds;
    Register *_regs;
    HostAHCIDevice::Register *_regs_high;
    size_t _portcount;
    HostAHCIDevice *_ports[32];
};
