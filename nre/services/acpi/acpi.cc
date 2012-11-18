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
#include <services/ACPI.h>
#include <services/PCIConfig.h>
#include <Logging.h>

#include "HostACPI.h"
#include "HostATARE.h"

using namespace nre;

static HostACPI *hostacpi;
static HostATARE *hostatare;

PORTAL static void portal_acpi(capsel_t) {
    UtcbFrameRef uf;
    try {
        ACPI::Command cmd;
        uf >> cmd;

        if(!hostacpi)
            throw Exception(E_NOT_FOUND, "No ACPI");

        switch(cmd) {
            case ACPI::FIND_TABLE: {
                String name;
                uint instance;
                uf >> name >> instance;
                uf.finish_input();

                const HostACPI::ACPIListItem *item = hostacpi->find(name.str(), instance);
                LOG(ACPI, "ACPI::FIND_TABLE '" << name << "' #" << instance
                                               << " -> " << (item ? "found" : "not found") << "\n");
                if(!item) {
                    VTHROW(Exception, E_NOT_FOUND,
                           "Unable to find APCI table '" << name << "' #" << instance);
                }
                uf.delegate(item->cap());
                uf << E_SUCCESS;
            }
            break;

            case ACPI::IRQ_TO_GSI: {
                uint irq;
                uf >> irq;
                uf.finish_input();

                uint gsi = hostacpi->irq_to_gsi(irq);
                LOG(ACPI, "ACPI::IRQ_TO_GSI " << irq << " -> " << gsi << "\n");
                uf << E_SUCCESS << gsi;
            }
            break;

            case ACPI::GET_GSI: {
                BDF bdf, parentbdf;
                uint8_t pin;
                uf >> bdf >> pin >> parentbdf;
                uf.finish_input();

                uint gsi = hostatare->get_gsi(bdf, pin, parentbdf);
                LOG(ACPI, "ACPI::GET_GSI" << " bdf=" << bdf << ", pin=" << pin
                                          << ", parent=" << parentbdf << " -> " << gsi << "\n");
                uf << E_SUCCESS << gsi;
            }
            break;
        }
    }
    catch(const Exception &e) {
        uf.clear();
        uf << e;
    }
}

int main() {
    try {
        hostacpi = new HostACPI();
        hostatare = new HostATARE(*hostacpi, 0);
    }
    catch(const Exception &e) {
        Serial::get() << e;
    }

    Service *srv = new Service("acpi", CPUSet(CPUSet::ALL), portal_acpi);
    srv->start();
    return 0;
}
