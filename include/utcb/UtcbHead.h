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

#pragma once

#include <arch/Types.h>

namespace nul {

class Utcb;
class UtcbFrame;
class UtcbFrameRef;
class OStream;
OStream &operator<<(OStream &os,const Utcb &utcb);
OStream &operator<<(OStream &os,const UtcbFrameRef &frm);

class UtcbHead {
	friend class UtcbFrame;
	friend class UtcbFrameRef;
	friend OStream &operator<<(OStream &os,const Utcb &utcb);
	friend OStream &operator<<(OStream &os,const UtcbFrameRef &frm);

protected:
	uint16_t top;
	uint16_t bottom;
	union {
		struct {
			uint16_t untyped;
			uint16_t typed;
		};
		word_t mtr;
	};
	word_t crd_translate;
	word_t crd;
	word_t nul_cpunr;
};

}
