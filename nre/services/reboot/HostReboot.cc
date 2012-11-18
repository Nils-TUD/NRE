/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
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

#include <Logging.h>

#include "HostReboot.h"

using namespace nre;

HostRebootKeyboard::HostRebootKeyboard() : HostRebootMethod(), _con("keyboard"), _sess(_con) {
}

void HostRebootKeyboard::reboot() {
    LOG(REBOOT, "Trying reboot via PS2 keyboard...\n");
    _sess.reboot();
}

HostRebootSysCtrlPortA::HostRebootSysCtrlPortA() : HostRebootMethod(), _port(0x92, 1) {
}

void HostRebootSysCtrlPortA::reboot() {
    LOG(REBOOT, "Trying reboot via System Control Port A...\n");
    _port.out<uint8_t>(0x01);
}

HostRebootPCIReset::HostRebootPCIReset() : HostRebootMethod(), _con("pcicfg"), _sess(_con) {
}

void HostRebootPCIReset::reboot() {
    LOG(REBOOT, "Trying reboot via PCI...\n");
    _sess.reboot();
}

HostRebootACPI::HostRebootACPI()
    : HostRebootMethod(), _method(), _value(), _addr(), _con("acpi"), _sess(_con), _ports(), _ds() {
    LOG(REBOOT, "Trying reboot via ACPI...\n");
    DataSpace table = _sess.find_table("FACP");
    ACPI::RSDT *rsdt = reinterpret_cast<ACPI::RSDT*>(table.virt());
    char *raw = reinterpret_cast<char*>(table.virt());
    if(rsdt->length < 129)
        VTHROW(Exception, E_NOT_FOUND, "FACP too small (" << rsdt->length << ")");
    if(~raw[113] & 0x4)
        throw Exception(E_NOT_FOUND, "Reset unsupported");
    if(raw[117] != 8)
        VTHROW(Exception, E_NOT_FOUND, "Register width invalid (" << raw[117] << ")");
    if(raw[118] != 0)
        VTHROW(Exception, E_NOT_FOUND, "Register offset invalid (" << raw[118] << ")");
    if(raw[119] > 1)
        throw Exception(E_NOT_FOUND, "Byte access needed");

    _method = raw[116];
    _value = raw[128];
    _addr = *reinterpret_cast<uint64_t*>(raw + 120);
    LOG(REBOOT, "Using method=" << fmt(_method, "#x") << ", value=" << fmt(_value, "#x")
                                << ", addr=" << fmt(_addr, "#x") << "\n");
    switch(_method) {
        case 0:
            _ds = new DataSpace(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::RW, _addr);
            break;
        case 1:
            if(_addr >= 0x10000)
                VTHROW(Exception, E_NOT_FOUND, "PIO out of range (" << _addr << ")");
            _ports = new Ports(_addr, 1);
            break;
        case 2:
            _ports = new Ports(0xcf8, 8);
            break;
        default:
            throw Exception(E_NOT_FOUND, "Wrong access type");
    }
}

HostRebootACPI::~HostRebootACPI() {
    delete _ports;
    delete _ds;
}

void HostRebootACPI::reboot() {
    switch(_method) {
        case 0:
            *reinterpret_cast<volatile uint8_t*>(_ds->virt()) = _value;
            break;
        case 1:
            _ports->out<uint8_t>(_value);
            break;
        case 2:
            uint32_t value = (_addr & 0x1f00000000ull) >> (32 - 11);
            value |= (_addr & 0x70000) >> (16 - 8);
            value |= _addr & 0x3c;
            _ports->out<uint32_t>(value, 0);
            _ports->out<uint8_t>(_value, 4 | (_addr & 0x3));
            break;
    }
}
