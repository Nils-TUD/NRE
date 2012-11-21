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

#pragma once

#include <arch/Types.h>
#include <services/ACPI.h>
#include <util/SList.h>
#include <Compiler.h>

class HostACPI {
    static const uintptr_t BIOS_MEM_ADDR    = 0xE0000;
    static const size_t BIOS_MEM_SIZE       = 0x20000;
    static const uintptr_t BIOS_ADDR        = 0x0;
    static const size_t BIOS_SIZE           = 0x1000;
    static const size_t BIOS_EBDA_OFF       = 0x40E;
    static const size_t BIOS_EBDA_SIZE      = 1024;

    // root system descriptor pointer
    struct RSDP {
        uint32_t signature[2];
        uint8_t checksum;
        char oemId[6];
        uint8_t revision;
        uint32_t rsdtAddr;
        // since 2.0
        uint32_t length;
        uint64_t xsdtAddr;
        uint8_t xchecksum;
    } PACKED;

    // APIC Structure (5.2.11.4)
    struct APIC {
        enum Type {
            LAPIC = 0, IOAPIC = 1, INTR = 2,
        };
        uint8_t type;
        uint8_t length;
    } PACKED;

    // Interrupt Source Override (5.2.11.8)
    struct APICIntr : public APIC {
        uint8_t bus;
        uint8_t irq;
        uint32_t gsi;
        uint16_t flags;
    } PACKED;

    // Multiple APIC Description Table
    struct MADT : public nre::ACPI::RSDT {
        uint32_t apic_addr;
        uint32_t flags;
        APIC apic[];
    } PACKED;

public:
    class ACPIListItem : public nre::SListItem {
    public:
        explicit ACPIListItem(const nre::ACPI::RSDT *tbl)
            : nre::SListItem(), _ds(tbl->length, nre::DataSpaceDesc::ANONYMOUS, nre::DataSpaceDesc::RW) {
            // TODO unfortunatly we have to make the dataspace writable to copy the content. this
            // means that our clients will get it writable as well since we have no concept of
            // restricting access writes to shared dataspaces.
            memcpy(reinterpret_cast<void*>(_ds.virt()), tbl, tbl->length);
        }

        capsel_t cap() const {
            return _ds.sel();
        }
        uintptr_t start() const {
            return _ds.virt();
        }
        uintptr_t end() const {
            return _ds.virt() + length();
        }
        size_t length() const {
            return table()->length;
        }
        const nre::ACPI::RSDT *table() const {
            return reinterpret_cast<nre::ACPI::RSDT*>(_ds.virt());
        }

    private:
        ACPIListItem(const ACPIListItem&);
        ACPIListItem &operator=(const ACPIListItem&);

        nre::DataSpace _ds;
    };

    explicit HostACPI();
    ~HostACPI() {
        for(auto it = _tables.begin(); it != _tables.end(); ) {
            auto old = it++;
            delete &*old;
        }
    }

    const ACPIListItem *find(const char *name, uint instance);
    uint irq_to_gsi(uint irq);

private:
    static nre::DataSpace map_table(uintptr_t addr, nre::ACPI::RSDT *&res);
    static char checksum(const char *table, unsigned count);
    static RSDP *get_rsdp();

private:
    nre::SList<ACPIListItem> _tables;
};
