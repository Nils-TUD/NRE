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

#include <utcb/UtcbFrame.h>

namespace nre {

OStream &operator<<(OStream &os, const UtcbFrameRef &frm) {
    os << "UtcbFrame @ " << frm._utcb << ":\n";
    os << "\tDelegate: " << Crd(frm._utcb->crd) << "\n";
    os << "\tTranslate: " << Crd(frm._utcb->crd_translate) << "\n";
    os << "\tUntyped: " << frm.untyped() << "\n";
    for(size_t i = 0; i < frm.untyped(); ++i)
        os << "\t\t" << i << ": " << fmt(frm._utcb->msg[i], "#x") << "\n";
    os << "\tTyped: " << frm.typed() << "\n";
    for(size_t i = 0; i < frm.typed() * 2; i += 2) {
        os << "\t\t" << i << ": "
           << fmt(frm._top[-(i + 1)], "#x") << " " << fmt(frm._top[-(i + 2)], "#x")
           << " @ " << &frm._top[-(i + 1)];
    }
    return os;
}

OStream & operator<<(OStream &os, const UtcbExc::Descriptor &desc) {
    os << "Desc[base=" << fmt(desc.base, "x") << " limit=" << fmt(desc.limit, "x")
       << " sel=" << fmt(desc.sel, "x") << " ar=" << fmt(desc.ar, "x") << "]";
    return os;
}

OStream & operator<<(OStream &os, const UtcbExcFrameRef &frm) {
    os << "UtcbExcFrame @ " << frm._utcb << ":\n";
    os << "\tMtd: " << Mtd(frm->mtd) << "\n";
    struct {
        const char *name;
        word_t val;
    } static words[] = {
        {"inst_len", frm->inst_len}, {"rip", frm->rip}, {"rfl", frm->rfl},
        {"cr0", frm->cr0}, {"cr2", frm->cr2}, {"cr3", frm->cr3},
        {"cr4", frm->cr4}, {"dr7", frm->dr7}, {"sysenter_cs", frm->sysenter_cs},
        {"sysenter_rsp", frm->sysenter_rsp}, {"sysenter_rip", frm->sysenter_rip}
    };
    for(size_t i = 0; i < ARRAY_SIZE(words); ++i)
        os << "\t" << words[i].name << ": " << fmt(words[i].val, "#0x", sizeof(words[i].val) * 2) << "\n";
    for(size_t i = 0; i < ARRAY_SIZE(frm->gpr); ++i)
        os << "\tgpr[" << i << "]: " << fmt(frm->gpr[i], "#0x", sizeof(frm->gpr[i]) * 2) << "\n";
    os << "\tintr_state: " << fmt(frm->intr_state, "#0x", 8) << "\n";
    os << "\tactv_state: " << fmt(frm->actv_state, "#0x", 8) << "\n";
    os << "\tinj_info: " << fmt(frm->inj_info, "#0x", 8) << "\n";
    os << "\tinj_error: " << fmt(frm->inj_error, "#0x", 8) << "\n";
    os << "\tqual[0]: " << fmt(frm->qual[0], "#0x", 16)
       << ", qual[1]: " << fmt(frm->qual[1], "#0x", 16) << "\n";
    os << "\tctrl[0]: " << fmt(frm->ctrl[0], "#0x", 8)
       << ", ctrl[1]: " << fmt(frm->ctrl[1], "#0x", 8) << "\n";
    os << "\ttsc_off: " << frm->tsc_off << "\n";
    struct {
        const char *name;
        const UtcbExc::Descriptor *desc;
    } static descs[] = {
        {"es", &frm->es}, {"cs", &frm->cs}, {"ss", &frm->ss}, {"ds", &frm->ds}, {"fs", &frm->fs},
        {"gs", &frm->gs}, {"ld", &frm->ld}, {"tr", &frm->tr}, {"gd", &frm->gd}, {"id", &frm->id},
    };
    for(size_t i = 0; i < ARRAY_SIZE(descs); ++i)
        os << "\t" << descs[i].name << ": " << *descs[i].desc << "\n";
    return os;
}

}
