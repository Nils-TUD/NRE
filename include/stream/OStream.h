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

#include <stdarg.h>
#include <Types.h>

namespace nul {

class OStream {
	enum {
		PADRIGHT	= 1 << 0,
		FORCESIGN	= 1 << 1,
		SPACESIGN	= 1 << 2,
		PRINTBASE	= 1 << 3,
		PADZEROS	= 1 << 4,
		CAPHEX		= 1 << 5,
		LONGLONG	= 1 << 6,
		LONG		= 1 << 7,
		SIZE_T		= 1 << 8,
		INTPTR_T	= 1 << 9,
	};

public:
	OStream() {
	}
	virtual ~OStream() {
	}

	void writef(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vwritef(fmt,ap);
		va_end(ap);
	}
	void vwritef(const char *fmt,va_list ap);

	virtual void write(char c) = 0;

private:
	void printnpad(llong n, uint pad, uint flags);
	void printupad(ullong u, uint base, uint pad, uint flags);
	int printpad(int count, uint flags);
	int printu(ullong n, uint base, char *chars);
	int printn(llong n);
	int puts(const char *str);

	static char _hexchars_big[];
	static char _hexchars_small[];
};

}
