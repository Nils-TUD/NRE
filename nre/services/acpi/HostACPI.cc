/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009-2010, Bernhard Kauer <bk@vmmon.org>
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

#include <mem/DataSpace.h>
#include <stream/Serial.h>
#include <Logging.h>
#include <String.h>
#include <cstring>

#include "HostACPI.h"

using namespace nre;

HostACPI::HostACPI() : _count(), _tables(), _rsdp(get_rsdp()) {
    // get rsdt
    DataSpace *ds;
    ds = new DataSpace(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, _rsdp->rsdtAddr);
    ACPI::RSDT *rsdt =
        reinterpret_cast<ACPI::RSDT*>(ds->virt() + (_rsdp->rsdtAddr & (ExecEnv::PAGE_SIZE - 1)));
    if(rsdt->length > ExecEnv::PAGE_SIZE) {
        uintptr_t size = rsdt->length;
        delete ds;
        ds = new DataSpace(size, DataSpaceDesc::LOCKED, DataSpaceDesc::R, _rsdp->rsdtAddr);
        rsdt = reinterpret_cast<ACPI::RSDT*>(ds->virt() + (_rsdp->rsdtAddr & (ExecEnv::PAGE_SIZE - 1)));
    }

    if(checksum(reinterpret_cast<char*>(rsdt), rsdt->length) != 0)
        throw Exception(E_NOT_FOUND, "RSDT checksum invalid");

    // find out the address range of all tables to map them contiguously
    uint32_t *tables = reinterpret_cast<uint32_t*>(rsdt + 1);
    uintptr_t min = 0xFFFFFFFF, max = 0;
    size_t count = (rsdt->length - sizeof(ACPI::RSDT)) / 4;
    for(size_t i = 0; i < count; i++) {
        DataSpace tmp(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, tables[i]);
        ACPI::RSDT *tbl =
            reinterpret_cast<ACPI::RSDT*>(tmp.virt() + (tables[i] & (ExecEnv::PAGE_SIZE - 1)));
        // determine the address range for the RSDT's
        if(tables[i] < min)
            min = tables[i];
        if(tables[i] + tbl->length > max)
            max = tables[i] + tbl->length;
    }

    // map them and put all pointers in an array
    size_t size = (max + ExecEnv::PAGE_SIZE * 2 - 1) - (min & ~(ExecEnv::PAGE_SIZE - 1));
    _ds = new DataSpace(size, DataSpaceDesc::LOCKED, DataSpaceDesc::R, min);
    _count = count;
    _tables = new ACPI::RSDT *[count];
    for(size_t i = 0; i < count; i++) {
        _tables[i] = reinterpret_cast<ACPI::RSDT*>(
            _ds->virt() + (min & (ExecEnv::PAGE_SIZE - 1)) - min + tables[i]);
        LOG(ACPI, "ACPI: found table " << fmt(_tables[i]->signature, 0U, 4) << "\n");
    }
}

uintptr_t HostACPI::find(const char *name, uint instance, size_t &length) {
    for(size_t i = 0; i < _count; i++) {
        if(memcmp(_tables[i]->signature, name, 4) == 0 && instance-- == 0) {
            if(checksum(reinterpret_cast<char*>(_tables[i]), _tables[i]->length) != 0)
                VTHROW(Exception, E_NOT_FOUND, "Checksum of ACPI table '" << name << "' invalid");
            length = _tables[i]->length;
            return reinterpret_cast<uintptr_t>(_tables[i]) - _ds->virt();
        }
    }
    return 0;
}

uint HostACPI::irq_to_gsi(uint irq) {
    size_t len;
    uintptr_t addr = find("APIC", 0, len);
    if(addr) {
        // search for interrupt source overrides in the MADT
        MADT *madt = reinterpret_cast<MADT*>(_ds->virt() + addr);
        for(APIC *apic = madt->apic;
            reinterpret_cast<uintptr_t>(apic) < _ds->virt() + addr + madt->length;
            apic = reinterpret_cast<APIC*>(reinterpret_cast<uintptr_t>(apic) + apic->length)) {
            if(apic->type == APIC::INTR) {
                APICIntr *iso = reinterpret_cast<APICIntr*>(apic);
                if(iso->irq == irq)
                    return iso->gsi;
            }
        }
    }
    // if not found, assume identity mapping
    return irq;
}

HostACPI::RSDP *HostACPI::get_rsdp() {
    // search in BIOS readonly memory range
    DataSpace *ds = new DataSpace(BIOS_MEM_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, BIOS_MEM_ADDR);
    char *area = reinterpret_cast<char*>(ds->virt());
    for(uintptr_t off = 0; off < BIOS_MEM_SIZE; off += 16) {
        if(memcmp(area + off, "RSD PTR ", 8) == 0 && checksum(area + off, 20) == 0) {
            LOG(ACPI, "ACPI: found RSDP in Bios readonly memory @ " << fmt(area + off, "p") << "\n");
            return reinterpret_cast<RSDP*>(area + off);
        }
    }
    delete ds;

    // search in ebda (+1 is necessary because 0 is considered as "give me arbitrary RAM")
    DataSpace bios(BIOS_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, BIOS_ADDR + 1);
    uint16_t ebda = *reinterpret_cast<uint16_t*>(bios.virt() + BIOS_EBDA_OFF);
    ds = new DataSpace(BIOS_EBDA_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, ebda << 4);
    area = reinterpret_cast<char*>(ds->virt() + ((ebda << 4) & (ExecEnv::PAGE_SIZE - 1)));
    for(uintptr_t off = 0; off < BIOS_EBDA_SIZE; off += 16) {
        if(memcmp(area + off, "RSD PTR ", 8) == 0 && checksum(area + off, 20) == 0) {
            LOG(ACPI, "ACPI: found RSDP in Bios EDBA @ " << fmt(area + off, "p") << "\n");
            return reinterpret_cast<RSDP*>(area + off);
        }
    }
    delete ds;

    throw Exception(E_NOT_FOUND, "Unable to find RSDP");
}
