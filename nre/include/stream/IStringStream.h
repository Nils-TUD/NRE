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

#include <stream/IStream.h>
#include <cstring>

namespace nre {

/**
 * Inputstream that read from a string
 */
class IStringStream : public IStream {
public:
	/**
	 * Reads a value of type <T> from the given string
	 *
	 * @param str the string
	 * @param len the length or -1 for "use strlen()"
	 * @return the read value
	 */
	template<typename T>
	static T read_from(const char *str, size_t len = (size_t)-1) {
		IStringStream is(str, len);
		T t;
		is >> t;
		return t;
	}

	/**
	 * Constructor
	 *
	 * @param str the string
	 * @param len the length or -1 for "use strlen()"
	 */
	explicit IStringStream(const char *str, size_t len = (size_t)-1)
		: IStream(), _str(str), _len(len == (size_t) - 1 ? strlen(str) : len), _pos() {
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
