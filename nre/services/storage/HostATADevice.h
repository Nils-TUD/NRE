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

#pragma once

#include <Compiler.h>
#include <Logging.h>

#include "Device.h"
#include "HostIDECtrl.h"

class HostATADevice : public Device {
public:
    enum Operation {
        READ,
        WRITE,
        PACKET
    };

    explicit HostATADevice(HostIDECtrl &ctrl, uint id, const Identify &info)
        : Device(id, info), _ctrl(ctrl),
          _buffer(nre::ExecEnv::PAGE_SIZE, nre::DataSpaceDesc::ANONYMOUS, nre::DataSpaceDesc::RW) {
    }
    virtual ~HostATADevice() {
    }

    virtual const char *type() const {
        return is_atapi() ? "ATAPI" : "ATA";
    }
    virtual void determine_capacity() {
        _capacity = has_lba48() ? _info.lba48MaxLBA : _info.userSectorCount;
    }
    virtual void readwrite(Operation op, const nre::DataSpace &ds, sector_type sector,
                           const dma_type &dma, producer_type *prod, tag_type tag, size_t secsize = 0);
    void flush_cache();

protected:
    uint8_t *buffer() const {
        return reinterpret_cast<uint8_t*>(_buffer.virt());
    }
    void transferPIO(Operation op, const nre::DataSpace &ds, size_t secsize, const dma_type &dma,
                     producer_type *prod, tag_type tag, bool waitfirst);
    void transferDMA(Operation op, const nre::DataSpace &ds, const dma_type &dma,
                     producer_type *prod, tag_type tag);

    uint get_command(Operation op);
    void setup_command(sector_type sector, sector_type count, uint cmd);

    HostIDECtrl &_ctrl;
    nre::DataSpace _buffer;
};
