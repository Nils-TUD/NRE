/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2009, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>
#include <services/VMManager.h>

#include "bus/motherboard.h"
#include "Timeouts.h"
#include "StorageDevice.h"
#include "VCPUBackend.h"

extern nre::UserSm globalsm;

class Vancouver : public StaticReceiver<Vancouver> {
public:
    explicit Vancouver(const char *args, size_t console, const nre::String &constitle)
        : _mb(), _timeouts(_mb), _conscon("console"), _conssess(_conscon, console, constitle),
          _stcon(), _vmmngcon(), _vmmng(), _vcpus(), _stdevs() {
        // storage is optional
        try {
            _stcon = new nre::Connection("storage");
        }
        catch(const nre::Exception &e) {
            nre::Serial::get() << "Unable to connect to storage: " << e.msg() << "\n";
        }
        create_devices(args);
        create_vcpus();

        nre::GlobalThread *input = nre::GlobalThread::create(
            keyboard_thread, nre::CPU::current().log_id(), nre::String("vmm-input"));
        input->set_tls<Vancouver*>(nre::Thread::TLS_PARAM, this);
        input->start();

        // vmmanager is optional
        try {
            _vmmngcon = new nre::Connection("vmmanager");
            _vmmng = new nre::VMManagerSession(*_vmmngcon);
            nre::GlobalThread *vmmng = nre::GlobalThread::create(
                vmmng_thread, nre::CPU::current().log_id(), nre::String("vmm-vmmng"));
            vmmng->set_tls<Vancouver*>(nre::Thread::TLS_PARAM, this);
            vmmng->start();
        }
        catch(const nre::Exception &e) {
            nre::Serial::get() << "Unable to connect to vmmanager: " << e.msg() << "\n";
        }
    }

    void reset();
    bool receive(CpuMessage &msg);
    bool receive(MessageHostOp &msg);
    bool receive(MessagePciConfig &msg);
    bool receive(MessageAcpi &msg);
    bool receive(MessageTimer &msg);
    bool receive(MessageTime &msg);
    bool receive(MessageLegacy &msg);
    bool receive(MessageConsoleView &msg);
    bool receive(MessageDisk &msg);

private:
    static void keyboard_thread(void*);
    static void vmmng_thread(void*);
    void create_devices(const char *args);
    void create_vcpus();

    Motherboard _mb;
    Timeouts _timeouts;
    nre::Connection _conscon;
    nre::ConsoleSession _conssess;
    nre::Connection *_stcon;
    nre::Connection *_vmmngcon;
    nre::VMManagerSession *_vmmng;
    nre::SList<VCPUBackend> _vcpus;
    StorageDevice *_stdevs[nre::Storage::MAX_CONTROLLER * nre::Storage::MAX_DRIVES];
};
