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

#include <utcb/UtcbHead.h>
#include <Syscalls.h>
#include <assert.h>

namespace nul {

class Utcb : public UtcbHead {
	friend class UtcbFrameRef;

	uint32_t msg[(4096 - sizeof(UtcbHead)) / sizeof(uint32_t)];
	enum {
		MAX_WORDS = sizeof(msg) / sizeof(uint32_t)
	};

public:
	void reset() {
		mtr = 0;
		crd = 0;
		crd_translate = 0;
	}
	void push() {
		reset();
		// TODO
	}
	void pop() {
		// TODO
	}

	void print(Format &fmt) const {
		fmt.print("Typed: %u\n",typed);
		for(size_t i = 0; i < typed; ++i)
			fmt.print("\t%zu: %#x\n",i,msg[i]);
		fmt.print("Untyped: %u\n",untyped);
		for(size_t i = 0; i < untyped; ++i)
			fmt.print("\t%zu: %#x\n",i,msg[Utcb::MAX_WORDS - i - 1]);
	}
};

}
