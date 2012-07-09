/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

class Ports {
public:
	template<typename T>
	static inline unsigned in(unsigned port) {
		T val;
		asm volatile ("in %w1, %0" : "=a" (val) : "Nd" (port));
		return val;
	}

    template<typename T>
	static inline void out(unsigned port,T val) {
		asm volatile ("out %0, %w1" : : "a" (val), "Nd" (port));
	}
};
