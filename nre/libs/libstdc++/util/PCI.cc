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

#include <util/PCI.h>
#include <Logging.h>

namespace nre {

Gsi *PCI::get_gsi_msi(BDF bdf, uint nr, void *msix_table) {
    size_t msix_offset = find_cap(bdf, CAP_MSIX);
    size_t msi_offset = find_cap(bdf, CAP_MSI);
    if(!(msix_offset || msi_offset))
        throw PCIException(E_FAILURE, "No MSI support");

    // determine device address and ensure that the memory is mapped in
    uintptr_t phys_addr = _pcicfg.addr(bdf, 0);
    DataSpace devds(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED, DataSpaceDesc::R, phys_addr);

    // create GSI
    Gsi *gsi = new Gsi(reinterpret_cast<void*>(devds.virt()), CPU::current().log_id());
    if(!gsi->msi_addr())
        throw PCIException(E_FAILURE, "Attach to MSI failed - IRQs may be broken!");

    // MSI-X
    if(msix_offset) {
        if(!msix_table) {
            value_type table_offset = conf_read(bdf, msix_offset + 1);
            uintptr_t base = bar_base(bdf, BAR0 + (table_offset & 0x7)) + (table_offset & ~0x7u);

            DataSpace msixbar(ExecEnv::PAGE_SIZE, DataSpaceDesc::LOCKED,
                              DataSpaceDesc::R, base);
            msix_table = reinterpret_cast<void*>(msixbar.virt() + (base & (ExecEnv::PAGE_SIZE - 1)));
            init_msix_table(msix_table, bdf, msix_offset, nr, gsi);
        }
        else
            init_msix_table(msix_table, bdf, msix_offset, nr, gsi);
    }
    else if(msi_offset) {
        value_type ctrl = conf_read(bdf, msi_offset);
        size_t base = msi_offset + 1;
        conf_write(bdf, base + 0, gsi->msi_addr());
        conf_write(bdf, base + 1, gsi->msi_addr() >> 32);
        if(ctrl & 0x800000)
            base += 1;
        conf_write(bdf, base + 1, gsi->msi_value());

        // we use only a single message and enable MSIs here
        conf_write(bdf, msi_offset, (ctrl & ~0x700000) | 0x10000);
    }
    return gsi;
}

Gsi *PCI::get_gsi(BDF bdf, uint nr, bool /*level*/, void *msix_table) {
    // If the device is MSI or MSI-X capable, don't use legacy interrupts.
    if(find_cap(bdf, CAP_MSIX) || find_cap(bdf, CAP_MSI))
        return get_gsi_msi(bdf, nr, msix_table);

    // we can't program vector > 0 when we only have legacy interrupts
    assert(nr == 0);
    assert(_acpi != nullptr);

    // normal GSIs -  ask atare
    value_type pin = conf_read(bdf, 0xf) >> 8;
    if(!pin)
        throw PCIException(E_NOT_FOUND, 32, "No IRQ PINs connected on BDF %#x", bdf);

    uint gsi;
    try {
        gsi = _acpi->get_gsi(bdf, pin - 1, _pcicfg.search_bridge(bdf));
    }
    catch(...) {
        // No clue which GSI is triggered - fall back to PIC irq
        gsi = conf_read(bdf, 0xf) & 0xff;
    }
    return new Gsi(gsi, CPU::current().log_id());
}

size_t PCI::find_cap(BDF bdf, cap_type id) {
    try {
        // capabilities supported?
        if((conf_read(bdf, 1) >> 16) & 0x10) {
            for(size_t offset = conf_read(bdf, 0xd);
                (offset != 0) && !(offset & 0x3);
                offset = conf_read(bdf, offset >> 2) >> 8) {
                if((conf_read(bdf, offset >> 2) & 0xFF) == id)
                    return offset >> 2;
            }
        }
    }
    catch(...) {
        // ignore
    }
    LOG(nre::Logging::PCI,
        nre::Serial::get() << "Unable to find cap " << fmt(id, "#x")
                           << " for bdf " << bdf << "\n");
    return 0;
}

size_t PCI::find_extended_cap(BDF bdf, cap_type id) {
    try {
        value_type header;
        size_t offset;
        if((find_cap(bdf, CAP_PCIE)) && (~0U != conf_read(bdf, 0x40))) {
            for(offset = 0x100, header = conf_read(bdf, offset >> 2);
                offset != 0;
                offset = header >> 20, header = conf_read(bdf, offset >> 2)) {
                if((header & 0xFFFF) == id)
                    return offset >> 2;
            }
        }
    }
    catch(...) {
        // ignore
    }
    LOG(nre::Logging::PCI,
        nre::Serial::get() << "Unable to find extended cap " << fmt(id, "#x")
                           << " for bdf " << bdf << "\n");
    return 0;
}

uint64_t PCI::bar_base(BDF bdf, size_t bar, value_type *type) {
    value_type val = conf_read(bdf, bar);
    if((val & BAR_IO) == BAR_IO) {
        // XXX
        if(type)
            *type = BAR_IO;
        return val & ~0x3;
    }
    else {
        if(type)
            *type = val & BAR_TYPE_MASK;
        switch(val & BAR_TYPE_MASK) {
            case BAR_TYPE_32B:
                return val & BAR_MEM_MASK;
            case BAR_TYPE_64B:
                return (static_cast<uint64_t>(conf_read(bdf, bar + 1)) << 32) | (val & BAR_MEM_MASK);
            default:
                return ~0ULL;
        }
    }
}

uint64_t PCI::bar_size(BDF bdf, size_t bar, bool *is64bit) {
    value_type old = conf_read(bdf, bar);
    uint64_t size = 0;

    if(is64bit)
        *is64bit = false;
    if((old & BAR_IO) == 1) {
        // I/O BAR
        conf_write(bdf, bar, 0xFFFFFFFFU);
        size = ((conf_read(bdf, bar) & BAR_IO_MASK) ^ 0xFFFFFFFFU) + 1;
    }
    else {
        // Memory BAR
        switch(old & BAR_TYPE_MASK) {
            case BAR_TYPE_32B:
                conf_write(bdf, bar, 0xFFFFFFFFU);
                size = ((conf_read(bdf, bar) & BAR_MEM_MASK) ^ 0xFFFFFFFFU) + 1;
                break;
            case BAR_TYPE_64B: {
                if(is64bit)
                    *is64bit = true;
                value_type old_hi = conf_read(bdf, bar + 1);
                conf_write(bdf, bar, 0xFFFFFFFFU);
                conf_write(bdf, bar + 1, 0xFFFFFFFFU);
                uint64_t bar_size = static_cast<uint64_t>(conf_read(bdf, bar + 1)) << 32;
                bar_size = (((bar_size | conf_read(bdf, bar)) & ~0xFULL) ^ ~0ULL) + 1;
                size = bar_size;
                conf_write(bdf, bar + 1, old_hi);
                break;
            }
            default:
                // Not Supported. Return 0.
                return 0;
        }
    }

    conf_write(bdf, bar, old);
    return size;
}

}
