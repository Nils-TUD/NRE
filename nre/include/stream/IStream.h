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

namespace nre {

/**
 * The input-stream is used to read formatted input from various sources. Subclasses have
 * to implement the method to actually read a character. This class provides the higher-level
 * stuff around it. Note that this class isn't finished yet, because we don't really have
 * formatted input.
 */
class IStream {
public:
	explicit IStream() : _last('\0') {
	}
	virtual ~IStream() {
	}

	IStream &operator>>(uchar &u) {
		return readu<uchar>(u);
	}
	IStream &operator>>(char &n) {
		return readn<char>(n);
	}
	IStream &operator>>(ushort &u) {
		return readu<ushort>(u);
	}
	IStream &operator>>(short &n) {
		return readn<short>(n);
	}
	IStream &operator>>(uint &u) {
		return readu<uint>(u);
	}
	IStream &operator>>(int &n) {
		return readn<int>(n);
	}
	IStream &operator>>(ulong &u) {
		return readu<ulong>(u);
	}
	IStream &operator>>(long &n) {
		return readn<long>(n);
	}
	IStream &operator>>(ullong &u) {
		return readu<ullong>(u);
	}
	IStream &operator>>(llong &n) {
		return readn<llong>(n);
	}

private:
	virtual char read() = 0;

	void skip_non_numeric() {
		if(_last == '\0')
			_last = read();
		while(!(_last == '-' || (_last >= '0' && _last <= '9')))
			_last = read();
	}

	template<typename T>
	IStream &readu(T &u) {
		uint base = 10;
		skip_non_numeric();
		u = 0;
		if(_last == '0' && ((_last = read()) == 'x' || _last == 'X')) {
			base = 16;
			_last = read();
		}
		while((_last >= '0' && _last <= '9') ||
				(base == 16 && ((_last >= 'a' && _last <= 'f') || (_last >= 'A' && _last <= 'F')))) {
			if(_last >= 'a' && _last <= 'f')
				u = u * base + _last + 10 - 'a';
			else if(_last >= 'A' && _last <= 'F')
				u = u * base + _last + 10 - 'A';
			else
				u = u * base + _last - '0';
			_last = read();
		}
		return *this;
	}

	template<typename T>
	IStream &readn(T &n) {
		bool neg = false;
		skip_non_numeric();

		if(_last == '-') {
			neg = true;
			_last = read();
		}

		n = 0;
		while(_last >= '0' && _last <= '9') {
			n = n * 10 + _last - '0';
			_last = read();
		}

		// switch sign?
		if(neg)
			n = -n;
		return *this;
	}

	char _last;
};

}
