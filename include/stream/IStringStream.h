/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <stream/IStream.h>
#include <cstring>

namespace nre {

class IStringStream : public IStream {
public:
	template<typename T>
	static T read_from(const char *str,size_t len = (size_t)-1) {
		IStringStream is(str,len);
		T t;
		is >> t;
		return t;
	}

	explicit IStringStream(const char *str,size_t len = (size_t)-1) : IStream(),
			_str(str), _len(len == (size_t)-1 ? strlen(str) : len), _pos() {
	}

private:
	virtual char read() {
		if(_pos < _len)
			return _str[_pos++];
		return '\0';
	}

	const char *_str;
	size_t _len;
	size_t _pos;
};

}
