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

#include <ipc/Service.h>
#include <ipc/Consumer.h>
#include <stream/Serial.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <String.h>

#include "Log.h"

using namespace nre;

BufferedLog BufferedLog::_inst INIT_PRIO_SERIAL;
Log Log::_inst INIT_PRIO_SERIAL;
Service *Log::_srv;
const char *Log::_colors[] = {
    "31", "32", "33", "34", "35", "36"
};

BufferedLog::BufferedLog() : BaseSerial(), _bufpos(0), _buf() {
    BaseSerial::_inst = this;
}

Log::Log() : BaseSerial(), _ports(PORT_BASE, 6), _sm(1), _ready(true) {
    _ports.out<uint8_t>(0x80, LCR);          // Enable DLAB (set baud rate divisor)
    _ports.out<uint8_t>(0x01, DLR_LO);       // Set divisor to 1 (lo byte) 115200 baud
    _ports.out<uint8_t>(0x00, DLR_HI);       //                  (hi byte)
    _ports.out<uint8_t>(0x03, LCR);          // 8 bits, no parity, one stop bit
    _ports.out<uint8_t>(0, IER);             // disable interrupts
    _ports.out<uint8_t>(7, FCR);
    _ports.out<uint8_t>(3, MCR);
}

void Log::start() {
    _srv = new Service("log", CPUSet(CPUSet::ALL), portal);
    _srv->start();
}

void Log::write(uint sessid, const char *line, size_t len) {
    ScopedLock<UserSm> guard(&_sm);
    *this << "\e[0;" << _colors[sessid % ARRAY_SIZE(_colors)] << "m[" << sessid << "] ";
    for(size_t i = 0; i < len; ++i) {
        char c = line[i];
        if(c != '\n')
            *this << c;
    }
    *this << "\e[0m\n";
}

void Log::portal(capsel_t pid) {
    ScopedLock<RCULock> guard(&RCU::lock());
    ServiceSession *sess = _srv->get_session<ServiceSession>(pid);
    UtcbFrameRef uf;
    try {
        String line;
        uf >> line;
        uf.finish_input();

        Log::get().write(sess->id() + 1, line.str(), line.length());
        uf << E_SUCCESS;
    }
    catch(const Exception &e) {
        uf.clear();
        uf << e;
    }
}
