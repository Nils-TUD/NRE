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

HostACPI::HostACPI() : _tables() {
    // get rsdt
    ACPI::RSDT *rsdt;
    DataSpace ds = map_table(get_rsdp()->rsdtAddr, rsdt);
    if(checksum(reinterpret_cast<char*>(rsdt), rsdt->length) != 0)
        throw Exception(E_NOT_FOUND, "RSDT checksum invalid");

    // find out the address range of all tables to map them contiguously
    uint32_t *tables = reinterpret_cast<uint32_t*>(rsdt + 1);
    size_t count = (rsdt->length - sizeof(ACPI::RSDT)) / 4;
    for(size_t i = 0; i < count; i++) {
        ACPI::RSDT *tbl;
        DataSpace rsdt = map_table(tables[i], tbl);
        _tables.append(new ACPIListItem(tbl));
        LOG(ACPI, "ACPI: found table " << fmt(tbl->signature, 0U, 4) << "\n");
    }
}

const HostACPI::ACPIListItem *HostACPI::find(const char *name, uint instance) {
    for(auto it = _tables.begin(); it != _tables.end(); ++it) {
        if(memcmp(it->table()->signature, name, 4) == 0 && instance-- == 0) {
            if(checksum(reinterpret_cast<const char*>(it->table()), it->length()) != 0)
                VTHROW(Exception, E_NOT_FOUND, "Checksum of ACPI table '" << name << "' invalid");
            return &*it;
        }
    }
    return nullptr;
}

uint HostACPI::irq_to_gsi(uint irq) {
    const ACPIListItem *item = find("APIC", 0);
    if(item) {
        // search for interrupt source overrides in the MADT
        const MADT *madt = reinterpret_cast<const MADT*>(item->table());
        for(const APIC *apic = madt->apic;
            reinterpret_cast<uintptr_t>(apic) < item->end();
            apic = reinterpret_cast<const APIC*>(reinterpret_cast<uintptr_t>(apic) + apic->length)) {
            if(apic->type == APIC::INTR) {
                const APICIntr *iso = reinterpret_cast<const APICIntr*>(apic);
                if(iso->irq == irq)
                    return iso->gsi;
            }
        }
    }
    // if not found, assume identity mapping
    return irq;
}

DataSpace HostACPI::map_table(uintptr_t addr, ACPI::RSDT *&res) {
    DataSpace ds(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, addr);
    res = reinterpret_cast<ACPI::RSDT*>(ds.virt() + (addr & (ExecEnv::PAGE_SIZE - 1)));
    size_t effective_length = res->length + (addr & (ExecEnv::PAGE_SIZE - 1));
    if(effective_length <= ExecEnv::PAGE_SIZE)
        return ds;

    DataSpace ds2(effective_length, DataSpaceDesc::LOCKED, DataSpaceDesc::R, addr);
    res = reinterpret_cast<ACPI::RSDT*>(ds2.virt() + (addr & (ExecEnv::PAGE_SIZE - 1)));
    return ds2;
}

char HostACPI::checksum(const char *table, unsigned count) {
    char res = 0;
    while(count--)
        res += table[count];
    return res;
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
