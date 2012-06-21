/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>

namespace nul {

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
		while(!(_last >= '0' && _last <= '9'))
			_last = read();
	}

	template<typename T>
	IStream &readu(T &u) {
		skip_non_numeric();
		u = 0;
		while(_last >= '0' && _last <= '9') {
			u = u * 10 + _last - '0';
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
