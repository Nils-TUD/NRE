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

#pragma once

#include <arch/Types.h>
#include <stdarg.h>

namespace nre {

/**
 * This class can be written into an OStream to apply formatting while using the stream operators
 */
template<typename T>
class Format {
public:
	/**
	 * Constructor
	 *
	 * @param fmt the format to use
	 * @param value the value
	 */
	explicit Format(const char *fmt,const T &value) : _fmt(fmt), _value(value) {
	}

	/**
	 * @return the format
	 */
	const char *fmt() const {
		return _fmt;
	}
	/**
	 * @return the value
	 */
	const T &value() const {
		return _value;
	}

private:
	const char *_fmt;
	const T &_value;
};

/**
 * The output-stream is used to write formatted output to various destinations. Subclasses have
 * to implement the method to actually write a character. This class provides the higher-level
 * stuff around it.
 */
class OStream {
	enum Flags {
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
		PRECISION	= 1 << 10,
	};

public:
	explicit OStream() {
	}
	virtual ~OStream() {
	}

	template<typename T>
	OStream &operator <<(const Format<T>& fmt) {
		writef(fmt.fmt(),fmt.value());
		return *this;
	}
	OStream &operator <<(char c) {
		write(c);
		return *this;
	}
	OStream &operator <<(uchar u) {
		return operator<<(static_cast<ullong>(u));
	}
	OStream &operator <<(short n) {
		return operator<<(static_cast<llong>(n));
	}
	OStream &operator <<(ushort u) {
		return operator<<(static_cast<ullong>(u));
	}
	OStream &operator <<(int n) {
		return operator<<(static_cast<llong>(n));
	}
	OStream &operator <<(uint u) {
		return operator<<(static_cast<ullong>(u));
	}
	OStream &operator <<(long n) {
		return operator<<(static_cast<llong>(n));
	}
	OStream &operator <<(ulong u) {
		return operator<<(static_cast<ullong>(u));
	}
	OStream &operator <<(llong n) {
		printn(n);
		return *this;
	}
	OStream &operator <<(ullong u) {
		printu(u,10,_hexchars_small);
		return *this;
	}
	OStream &operator <<(const char *str) {
		puts(str,-1);
		return *this;
	}
	OStream &operator <<(const void *p) {
		printptr(reinterpret_cast<uintptr_t>(p),0);
		return *this;
	}

	void writef(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vwritef(fmt,ap);
		va_end(ap);
	}
	void vwritef(const char *fmt,va_list ap);

private:
	virtual void write(char c) = 0;

	void printnpad(llong n,uint pad,uint flags);
	void printupad(ullong u,uint base,uint pad,uint flags);
	int printpad(int count,uint flags);
	int printu(ullong n,uint base,char *chars);
	int printn(llong n);
	void printptr(uintptr_t u,uint flags);
	int puts(const char *str,ulong prec);

	static char _hexchars_big[];
	static char _hexchars_small[];
};

}
