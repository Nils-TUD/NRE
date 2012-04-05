/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ex/Exception.h>

class AssertException : public Exception {
public:
	explicit AssertException(const char *code,const char *file,int line) throw()
		: Exception(), _code(code), _file(file), _line(line) {
	}
	AssertException(const AssertException& ae)
		: Exception(ae), _code(ae._code), _file(ae._file), _line(ae._line) {
	}
	AssertException& operator=(const AssertException& ae) {
		if(&ae == this)
			return *this;
		Exception::operator=(ae);
		_code = ae._code;
		_file = ae._file;
		_line = ae._line;
		return *this;
	}
	virtual ~AssertException() throw() {
	}

	const char *code() const throw() {
		return _code;
	}
	const char *file() const throw() {
		return _file;
	}
	int line() const throw() {
		return _line;
	}

	virtual void print(Format& fmt) const {
		fmt.print("Assert '%s' failed in %s, line %d\n",code(),file(),line());
		print_backtrace(fmt);
	}

private:
	const char *_code;
	const char *_file;
	int _line;
};
