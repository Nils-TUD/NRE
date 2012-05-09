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

#include <arch/SpinLock.h>
#include <cstdlib>

static char buffer[8192];
static size_t pos = 0;
static uint lck = 0;

void* malloc(size_t size) {
	lock(&lck);
	void* res;
	if(pos + size >= sizeof(buffer)) {
		unlock(&lck);
		return 0;
	}
	
	res = buffer + pos;
	pos += size;
	unlock(&lck);
	return res;
}

void free(void* p) {
	// do nothing for now
}
