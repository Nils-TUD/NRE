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

#include <ex/SyscallException.h>
#include <Errors.h>
#include <arch/Types.h>

namespace nul {

class SyscallABI {
public:
	typedef uint64_t arg_t;

	static inline void syscall(arg_t w0) {
		asm volatile (
			"syscall"
			: "+D" (w0)
			:
			: "rcx", "r11", "memory"
		);
		handle_result(w0);
	}

	static inline void syscall(arg_t w0,arg_t w1) {
		asm volatile (
			"syscall"
			: "+D" (w0)
			: "S" (w1)
			: "rcx", "r11", "memory"
		);
		handle_result(w0);
	}

	static inline void syscall(arg_t w0,arg_t w1,arg_t w2) {
		asm volatile (
			"syscall"
			: "+D" (w0)
			: "S" (w1), "d" (w2)
			: "rcx", "r11", "memory"
		);
		handle_result(w0);
	}

	static inline void syscall(arg_t w0,arg_t w1,arg_t w2,arg_t w3) {
		asm volatile (
			"syscall"
			: "+D" (w0)
			: "S" (w1), "d" (w2), "a" (w3)
			: "rcx", "r11", "memory"
		);
		handle_result(w0);
	}

	static inline void syscall(arg_t w0,arg_t w1,arg_t w2,arg_t w3,arg_t w4) {
	    register arg_t r8 asm ("r8") = w4;
		asm volatile (
			"syscall"
			: "+D" (w0)
			: "S" (w1), "d" (w2), "a" (w3), "r" (r8)
			: "rcx", "r11", "memory"
		);
		handle_result(w0);
	}

private:
	static inline void handle_result(uint8_t res) {
		if(EXPECT_FALSE(res != ESUCCESS))
			throw SyscallException(static_cast<ErrorCode>(res));
	}
};

}
