/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
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
	Exception() throw();
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
