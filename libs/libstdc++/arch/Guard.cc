/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <arch/Types.h>
#include <arch/Defines.h>
#include <Compiler.h>

/**
 * GCC calls the __cxa_guard_-stuff when initializing local static objects to do it in a
 * thread-safe way. We use a simple cmp-and-swap based implementation to ensure that only one
 * thread initializes an object. This way we don't depend on anything and can use it therefore
 * for early initialization.
 */

namespace __cxxabiv1 {

// The ABI requires a 64-bit type
typedef uint64_t guard_t;

static inline int trylock(guard_t *l) {
	int res = 0;
	asm volatile (
		// res = 0 (gcc assumes that we assign it within the inline-assembly and ignores res = 0 above)
		"xor	%0,%0;"
		// try to exchange lock with 0x100 (keep first byte 0)
		"mov	$0x100,%%" EXPAND(REG(cx)) ";"
		"xor	%%" EXPAND(REG(ax)) ",%%" EXPAND(REG(ax)) ";"
		"lock	cmpxchg %%" EXPAND(REG(cx)) ",(%1);"
		// if it succeeded, the zero-flag is set
		"jnz	1f;"
		// in this case we report success
		"mov	$1,%0;"
		"1:;"
		: "=S"(res) : "D" (l) : EXPAND(REG(ax)), EXPAND(REG(cx)), "cc", "memory"
	);
	if(!res) {
		// wait until the first one has completely initialized the static object
		while(*(volatile char*)l != 1)
			asm volatile ("pause");
	}
	return res;
}
static inline void unlock(guard_t *l) {
	// unlock it and mark it initialized
	*(char*)l = 1;
}

EXTERN_C int __cxa_guard_acquire(guard_t *);
EXTERN_C void __cxa_guard_release(guard_t *);
EXTERN_C void __cxa_guard_abort(guard_t *);

int __cxa_guard_acquire(guard_t *g) {
	return trylock(g);
}

void __cxa_guard_release(guard_t *g) {
	unlock(g);
}

void __cxa_guard_abort(guard_t *) {
}

}
