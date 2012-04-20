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

#include <arch/ExecutionEnv.h>
#include <Types.h>
#include <Format.h>

namespace nul {

class Exception {
	enum {
		MAX_TRACE_DEPTH = 16
	};

public:
	typedef const uintptr_t *backtrace_iterator;

public:
	explicit Exception(const char *msg = 0) throw() : _msg(msg), _backtrace(), _count() {
		_count = ExecutionEnv::collect_backtrace(&_backtrace[0],MAX_TRACE_DEPTH);
	}
	virtual ~Exception() throw() {
	}

	backtrace_iterator backtrace_begin() const throw() {
		return _backtrace;
	}
	backtrace_iterator backtrace_end() const throw() {
		return _backtrace + _count;
	}

	virtual void print(Format& fmt) const {
		if(_msg) {
			fmt.print(_msg);
			fmt.print("\n");
		}
		print_backtrace(fmt);
	}

protected:
	void print_backtrace(Format& fmt) const {
		fmt.print("Backtrace:\n");
		for(backtrace_iterator it = backtrace_begin(); it != backtrace_end(); ++it)
			fmt.print("\t%p\n",*it);
	}

private:
	const char *_msg;
	uintptr_t _backtrace[MAX_TRACE_DEPTH];
	size_t _count;
};

}
