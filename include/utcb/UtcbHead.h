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

#include <Types.h>

namespace nul {

class UtcbFrame;
class UtcbFrameRef;

class UtcbHead {
	friend class UtcbFrame;
	friend class UtcbFrameRef;

protected:
	uint16_t top;
	uint16_t bottom;
	union {
		struct {
			uint16_t untyped;
			uint16_t typed;
		};
		uint32_t mtr;
	};
	uint32_t crd_translate;
	uint32_t crd;
	uint32_t nul_cpunr;
};

}
