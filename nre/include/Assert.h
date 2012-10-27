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

#include <Exception.h>

namespace nre {

/**
 * Special exception for asserts to print the expression that has been used, the file and the line.
 */
class AssertException : public Exception {
public:
	/**
	 * Constructor
	 *
	 * @param expr the expression
	 * @param file the filename
	 * @param line the line number
	 */
	explicit AssertException(const char *expr, const char *file, int line) throw()
		: Exception(E_ASSERT), _expr(expr), _file(file), _line(line) {
	}
	virtual ~AssertException() throw() {
	}

	/**
	 * @return the expression that has been asserted
	 */
	const char *expr() const throw() {
		return _expr;
	}
	/**
	 * @return the file in which the assert occurred
	 */
	const char *file() const throw() {
		return _file;
	}
	/**
	 * @return the line in which the assert occurred
	 */
	int line() const throw() {
		return _line;
	}

	virtual void write(OStream &os) const {
		os << "Assert '" << expr() << "' failed in " << file() << ", line " << line() << "\n";
		write_backtrace(os);
	}

private:
	const char *_expr;
	const char *_file;
	int _line;
};

}

#ifndef NDEBUG

#    define assert(cond)                                                    \
	do {                                                                    \
		if(!(cond)) {                                                       \
			throw nre::AssertException(# cond, __FILE__, __LINE__);         \
		}                                                                   \
	} while(0);

#else

#    define assert(cond)

#endif
