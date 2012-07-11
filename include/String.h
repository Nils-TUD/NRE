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
#include <stream/OStream.h>
#include <cstring>

namespace nre {

class String {
public:
	explicit String() : _str(0), _len() {
	}
	explicit String(const char *str,size_t len = static_cast<size_t>(-1))
		: _str(), _len() {
		init(str,len);
	}
	explicit String(const String& s)
		: _str(), _len() {
		init(s._str,s._len);
	}
	String& operator=(const String& s) {
		if(&s != this)
			reset(s._str,s._len);
		return *this;
	}
	~String() {
		delete _str;
	}

	const char *str() const {
		return _str ? _str : "";
	}
	size_t length() const {
		return _len;
	}
	void reset(const char *str,size_t len = static_cast<size_t>(-1)) {
		delete _str;
		init(str,len);
	}

private:
	void init(const char *str,size_t len) {
		_len = len == static_cast<size_t>(-1) ? strlen(str) : len;
		_str = new char[_len + 1];
		memcpy(_str,str,_len);
		_str[_len] = '\0';
	}

	char *_str;
	size_t _len;
};

static inline OStream &operator <<(OStream &os,const String &str) {
	return os << str.str();
}

}
