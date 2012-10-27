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
		os.writef("\t\t%zu: %#lx\n", i, frm._utcb->msg[i]);
	os << "\tTyped: " << frm.typed() << "\n";
	for(size_t i = 0; i < frm.typed() * 2; i += 2) {
		os.writef("\t\t%zu: %#lx %#lx @ %p\n", i, frm._top[-(i + 1)], frm._top[-(i + 2)],
		          &frm._top[-(i + 1)]);
	}
	return os;
}

OStream & operator<<(OStream &os, const UtcbExc::Descriptor &desc) {
	os.writef("Desc[base=%lx limit=%x sel=%x ar=%x]", desc.base, desc.limit, desc.sel, desc.ar);
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
		os.writef("\t%s: %#0" FMT_WORD_HEXLEN "lx\n", words[i].name, words[i].val);
	for(size_t i = 0; i < ARRAY_SIZE(frm->gpr); ++i)
		os.writef("\tgpr[%zu]: %#0" FMT_WORD_HEXLEN "lx\n", i, frm->gpr[i]);
	os.writef("\tintr_state: %#08x\n", frm->intr_state);
	os.writef("\tactv_state: %#08x\n", frm->actv_state);
	os.writef("\tinj_info: %#08x\n", frm->inj_info);
	os.writef("\tinj_error: %#08x\n", frm->inj_error);
	os.writef("\tqual[0]: %#016Lx, qual[1]: %#016Lx\n", frm->qual[0], frm->qual[1]);
	os.writef("\tctrl[0]: %#08x, ctrl[1]: %#08x\n", frm->ctrl[0], frm->ctrl[1]);
	os.writef("\ttsc_off: %Ld\n", frm->tsc_off);
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
