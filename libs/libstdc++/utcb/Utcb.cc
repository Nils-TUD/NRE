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

#include <utcb/Utcb.h>
#include <utcb/UtcbFrame.h>

namespace nul {

void Utcb::write(OStream &os) const {
	os.writef("Utcb @ %p:\n",this);
	os.writef("top: %u\n",top);
	os.writef("bottom: %u\n",bottom);
	uint16_t boff = bottom;
	uint16_t toff = top;
	while(1) {
		Utcb *utcb = reinterpret_cast<Utcb*>(reinterpret_cast<uintptr_t>(this) + boff * sizeof(word_t));
		UtcbFrameRef frame(utcb,toff);
		frame.write(os);
		if(boff == 0)
			break;
		boff = frame._utcb->bottom;
		toff = frame._utcb->top;
	}
}

}
