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

#include <stream/OStream.h>
#include <Desc.h>

namespace nre {

OStream &operator<<(OStream &os, const Crd &crd) {
    static const char *types[] = {"NULL", "MEM", "IO", "OBJ"};
    os << "Crd[type=" << types[crd.attr() & 0x3] << " offset=" << fmt(crd.offset(), "#x")
       << " order=" << fmt(crd.order(), "#x") << " attr=" << fmt(crd.attr(), "#x") << "]";
    return os;
}

OStream &operator<<(OStream &os, const Mtd &mtd) {
    static const char *flags[] = {
        "GPR_ACDB", "GPR_BSD", "RSP", "RIP_LEN", "RFLAGS", "DS_ES", "FS_GS", "CS_SS", "TR", "LDTR",
        "GDTR", "IDTR", "CR", "DR", "SYSENTER", "QUAL", "CTRL", "INJ", "STATE", "TSC"
    };
    os << "Mtd[";
    bool first = true;
    for(size_t i = 0; i < ARRAY_SIZE(flags); ++i) {
        if(mtd.value() & (1 << i)) {
            if(!first)
                os << " ";
            os << flags[i];
            first = false;
        }
    }
    os << "]";
    return os;
}

OStream &operator<<(OStream &os, const Qpd &qpd) {
    os << "Qpd[prio=" << qpd.prio() << " quantum=" << qpd.quantum() << "]";
    return os;
}

}
