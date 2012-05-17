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

#include <arch/Types.h>

#define MAX_EXIT_FUNCS		32

typedef void (*fRegFrameInfo)(void *callback);
typedef void (*fConstr)(void);
typedef struct {
	void (*f)(void*);
	void *p;
	void *d;
} sGlobalObj;

/**
 * Will be called by gcc at the beginning for every global object to register the
 * destructor of the object
 */
int __cxa_atexit(void (*f)(void *),void *p,void *d);
/**
 * We'll call this function in exit() to call all destructors registered by *atexit()
 */
void __cxa_finalize(void *d);

static size_t exitFuncCount = 0;
static sGlobalObj exitFuncs[MAX_EXIT_FUNCS];

int __cxa_atexit(void (*f)(void *),void *p,void *d) {
	if(exitFuncCount >= MAX_EXIT_FUNCS)
		return -1;

	exitFuncs[exitFuncCount].f = f;
	exitFuncs[exitFuncCount].p = p;
	exitFuncs[exitFuncCount].d = d;
	exitFuncCount++;
	return 0;
}

void __cxa_finalize(void *d) {
	ssize_t i;
	for(i = exitFuncCount - 1; i >= 0; i--)
		exitFuncs[i].f(exitFuncs[i].p);
}
