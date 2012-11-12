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

#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <services/Storage.h>

#include "bus/motherboard.h"
#include "bus/message.h"

class StorageDevice {
public:
    explicit StorageDevice(DBus<MessageDiskCommit> &bus, nre::DataSpace &guestmem,
                           nre::Connection &con, size_t no)
        : _no(no), _bus(bus), _con(con), _sess(_con, guestmem, no) {
        char buffer[32];
        nre::OStringStream os(buffer, sizeof(buffer));
        os << "vmm-storage-" << no;
        nre::GlobalThread *gt = nre::GlobalThread::create(
            thread, nre::CPU::current().log_id(), buffer);
        gt->set_tls<StorageDevice*>(nre::Thread::TLS_PARAM, this);
        gt->start();
    }

    void get_params(nre::Storage::Parameter *params) {
        *params = _sess.get_params();
    }
    void read(nre::Storage::tag_type tag, nre::Storage::sector_type sector,
              const nre::Storage::dma_type *dma) {
        _sess.read(tag, sector, *dma);
    }
    void write(nre::Storage::tag_type tag, nre::Storage::sector_type sector,
               const nre::Storage::dma_type *dma) {
        _sess.write(tag, sector, *dma);
    }
    void flush_cache(nre::Storage::tag_type tag) {
        _sess.flush(tag);
    }

private:
    static void thread(void*) {
        StorageDevice *sd = nre::Thread::current()->get_tls<StorageDevice*>(nre::Thread::TLS_PARAM);
        while(1) {
            nre::Storage::Packet *pk = sd->_sess.consumer().get();
            // the status isn't used anyway
            {
                nre::ScopedLock<nre::UserSm> guard(&globalsm);
                MessageDiskCommit msg(sd->_no, pk->tag, MessageDisk::DISK_OK);
                sd->_bus.send(msg);
            }
            sd->_sess.consumer().next();
        }
    }

    size_t _no;
    DBus<MessageDiskCommit> &_bus;
    nre::Connection &_con;
    nre::StorageSession _sess;
};
