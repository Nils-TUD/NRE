/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Types.h>

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
	~Digits();
	Digits(const Digits&);
	Digits& operator=(const Digits&);
};
