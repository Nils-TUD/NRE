/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <utcb/UtcbFrame.h>

namespace nul {

OStream &operator<<(OStream &os,const UtcbFrameRef &frm) {
	os << "UtcbFrame @ " << frm._utcb << ":\n";
	os << "\tDelegate: " << Crd(frm._utcb->crd) << "\n";
	os << "\tTranslate: " << Crd(frm._utcb->crd_translate) << "\n";
	os << "\tUntyped: " << frm.untyped() << "\n";
	for(size_t i = 0; i < frm.untyped(); ++i)
		os.writef("\t\t%zu: %#x\n",i,frm._utcb->msg[i]);
	os << "\tTyped: " << frm.typed() << "\n";
	for(size_t i = 0; i < frm.typed() * 2; i += 2)
		os.writef("\t\t%zu: %#x %#x @ %p\n",i,frm._top[-(i + 1)],frm._top[-(i + 2)],&frm._top[-(i + 1)]);
	return os;
}

}
