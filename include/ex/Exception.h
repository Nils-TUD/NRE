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

#include <Types.h>
#include <Format.h>

class Exception {
private:
	enum {
		MAX_TRACE_DEPTH = 16
	};

public:
	typedef const uintptr_t *backtrace_iterator;

public:
	explicit Exception() throw();
	virtual ~Exception() throw() {
	}

	backtrace_iterator backtrace_begin() const throw() {
		return _backtrace;
	}
	backtrace_iterator backtrace_end() const throw() {
		return _backtrace + _count;
	}

	virtual void print(Format& fmt) const = 0;

protected:
	void print_backtrace(Format& fmt) const {
		fmt.print("Backtrace:\n");
		for(Exception::backtrace_iterator it = backtrace_begin(); it != backtrace_end(); ++it)
			fmt.print("\t%p\n",*it);
	}

private:
	uintptr_t _backtrace[MAX_TRACE_DEPTH];
	size_t _count;
};
