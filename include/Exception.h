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

#include <arch/ExecEnv.h>
#include <arch/Types.h>
#include <stream/OStream.h>
#include <Errors.h>
#include <Compiler.h>

namespace std {
	/**
	 * Type of terminate-handlers
	 */
	typedef void (*terminate_handler)(void);
	/**
	 * Type of unexpected-handlers
	 */
	typedef void (*unexpected_handler)(void);

	/**
	 * Sets the terminate-handler
	 *
	 * @param pHandler the new one
	 * @return the old one
	 */
	terminate_handler set_terminate(terminate_handler pHandler) throw ();
	/**
	 * Is called by the runtime if exception-handling must be aborted
	 */
	void terminate(void) NORETURN;
	/**
	 * Sets the unexpected-handler
	 *
	 * @param pHandler the new one
	 * @return the old one
	 */
	unexpected_handler set_unexpected(unexpected_handler pHandler) throw ();
	/**
	 * Is called by the runtime if an exception is thrown which violates the functions exception
	 * specification
	 */
	void unexpected(void) NORETURN;
	/**
	 * @return true if the caught exception violates the throw specification.
	 */
	bool uncaught_exception() throw ();
}

namespace nul {

class Exception {
	enum {
		MAX_TRACE_DEPTH = 16
	};

public:
	typedef const uintptr_t *backtrace_iterator;

public:
	explicit Exception(ErrorCode code,const char *msg = 0) throw()
		: _code(code), _msg(msg), _backtrace(), _count() {
		_count = ExecEnv::collect_backtrace(&_backtrace[0],MAX_TRACE_DEPTH);
	}
	virtual ~Exception() throw() {
	}

	const char *msg() const {
		return _msg;
	}
	const char *name() const {
		return to_string(_code);
	}
	ErrorCode code() const {
		return _code;
	}
	backtrace_iterator backtrace_begin() const {
		return _backtrace;
	}
	backtrace_iterator backtrace_end() const {
		return _backtrace + _count;
	}

	virtual void write(OStream& os) const {
		os.writef("Exception: %s (%d)",name(),code());
		if(msg())
			os.writef(": %s",msg());
		os.writef("\n");
		write_backtrace(os);
	}

protected:
	void write_backtrace(OStream& os) const {
		os.writef("Backtrace:\n");
		for(backtrace_iterator it = backtrace_begin(); it != backtrace_end(); ++it)
			os.writef("\t%p\n",*it);
	}

private:
	ErrorCode _code;
	const char *_msg;
	uintptr_t _backtrace[MAX_TRACE_DEPTH];
	size_t _count;
	static const char *_msgs[];
};

}
