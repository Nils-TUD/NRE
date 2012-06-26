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

class Digits {
public:
	template<typename T>
	static uint count_unsigned(T n,uint base) {
		uint width = 1;
		while(n >= base) {
			n /= base;
			width++;
		}
		return width;
	}

	template<typename T>
	static uint count_signed(T n,uint base) {
		// we have at least one char
		uint width = 1;
		if(n < 0) {
			width++;
			n = -n;
		}
		while(n >= base) {
			n /= base;
			width++;
		}
		return width;
	}

private:
	Digits();
};

}
