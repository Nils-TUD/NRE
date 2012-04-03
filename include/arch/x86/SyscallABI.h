/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ex/SyscallException.h>
#include <Errors.h>
#include <Types.h>

class SyscallABI {
public:
	typedef uint32_t arg_t;

	/**
	 * A fast syscall with only two parameters.
	 */
	static inline void syscall(arg_t w0) {
		asm volatile (
			"mov %%esp, %%ecx;"
			"mov $1f, %%edx;"
			"sysenter;"
			"1: ;"
			: "+a" (w0)
			:
			: "ecx", "edx", "memory"
		);
		w0 &= 0xFF;
		if(w0 != ESUCCESS)
			throw SyscallException(static_cast<ErrorCode>(w0));
	}

	/**
	 * The default syscall
	 */
	static inline void syscall(arg_t w0,arg_t w1,arg_t w2,arg_t w3,arg_t w4,
			arg_t *out1 = 0,arg_t *out2 = 0) {
		asm volatile (
			"push %%ebp;"
			"mov %%ecx, %%ebp;"
			"mov %%esp, %%ecx;"
			"mov $1f, %%edx;"
			"sysenter;"
			"1: ;"
			"pop %%ebp;"
			: "+a" (w0), "+D" (w1), "+S" (w2), "+c"(w4)
			: "b"(w3)
			: "edx", "memory"
		);
		w0 &= 0xFF;
		if(w0 != ESUCCESS)
			throw SyscallException(static_cast<ErrorCode>(w0));
		if(out1)
			*out1 = w1;
		if(out2)
			*out2 = w2;
	}
};
