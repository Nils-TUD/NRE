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

namespace nul {

class AssertException : public Exception {
public:
	explicit AssertException(const char *expr,const char *file,int line) throw()
		: Exception(E_ASSERT), _expr(expr), _file(file), _line(line) {
	}
	virtual ~AssertException() throw() {
	}

	const char *expr() const throw() {
		return _expr;
	}
	const char *file() const throw() {
		return _file;
	}
	int line() const throw() {
		return _line;
	}

private:
	const char *_expr;
	const char *_file;
	int _line;
};

static inline OStream &operator<<(OStream &os,const AssertException &e) {
	os << "Assert '" << e.expr() << "' failed in " << e.file() << ", line " << e.line() << "\n";
	e.write_backtrace(os);
	return os;
}

}

#ifndef NDEBUG

#	define assert(cond) do { if(!(cond)) { \
			throw nul::AssertException(#cond,__FILE__,__LINE__); \
		} } while(0);

#else

#	define assert(cond)

#endif
