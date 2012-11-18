/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
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

#include <services/ACPI.h>
#include <Logging.h>

#include "HostMMConfig.h"

using namespace nre;

HostMMConfig::HostMMConfig() {
    Connection con("acpi");
    ACPISession sess(con);
    DataSpace table = sess.find_table("MCFG");
    AcpiMCFG *mcfg = reinterpret_cast<AcpiMCFG*>(table.virt());
    size_t count = (mcfg->len - sizeof(AcpiMCFG)) / sizeof(AcpiMCFG::Entry);
    for(size_t i = 0; i < count; ++i) {
        AcpiMCFG::Entry *entry = mcfg->entries + i;
        LOG(PCICFG, "MMConfig:" << " base " << fmt(entry->base, "#x")
                                << " seg " << fmt(entry->pci_seg, "#0x", 2)
                                << " bus " << fmt(entry->pci_bus_start, "#0x", 2)
                                << "-" << fmt(entry->pci_bus_end, "#0x", 2) << "\n");

        uint start = (entry->pci_seg << 16) + entry->pci_bus_start * 32 * 8;
        uint buses = entry->pci_bus_end - entry->pci_bus_start + 1;
        _ranges.append(new MMConfigRange(entry->base, start, buses * 32 * 8));
    }
}
