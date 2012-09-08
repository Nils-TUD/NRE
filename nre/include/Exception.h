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

#include <arch/ExecEnv.h>
#include <arch/Types.h>
#include <stream/OStream.h>
#include <Errors.h>
#include <Compiler.h>
#include <String.h>

/**
 * Defines all constructors of Exception for subclasses of it. I think, this is the best solution
 * atm since gcc does not support constructor inheritance yet. And without it, as you can see, it's
 * quite inconvenient to define the constructors for every class because of the variable argument
 * constructor. An alternative would be to use only Exception and no subclasses. But then we have
 * no way to distinguish exceptions and provide different catch blocks for them. Another way would
 * be to use the c++11-feature variadic templates to change the whole va_* stuff. But this is quite
 * radical and I'm not sure yet if we want that.
 * So, by providing this macro we can keep different exception classes with little effort and can
 * switch easily to the constructor inheritance concept as soon as its available in gcc.
 */
#define DEFINE_EXCONSTRS(className) \
	explicit className(ErrorCode code = E_FAILURE,const char *msg = 0) throw() : Exception(code,msg) {	\
	}																												\
	explicit className(ErrorCode code,size_t bufsize,const char *fmt,...) throw() : Exception(code) {		\
		va_list ap;																									\
		va_start(ap,fmt);																							\
		_msg.vformat(bufsize,fmt,ap);																				\
		va_end(ap);																									\
	}																												\
	explicit className(ErrorCode code,const String &msg) throw() : Exception(code,msg) {					\
	}

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

namespace nre {

class Exception;
class UtcbFrameRef;
static inline OStream &operator<<(OStream &os,const Exception &e);
UtcbFrameRef &operator<<(UtcbFrameRef &uf,const Exception &e);
UtcbFrameRef &operator>>(UtcbFrameRef &uf,Exception &e);

/**
 * The base class of all exceptions. All exceptions have an error-code, collect a backtrace and
 * have optionally a message (string).
 */
class Exception {
	friend UtcbFrameRef &operator<<(UtcbFrameRef &uf,const Exception &e);
	friend 	UtcbFrameRef &operator>>(UtcbFrameRef &uf,Exception &e);

	enum {
		MAX_TRACE_DEPTH = 16
	};

public:
	typedef const uintptr_t *backtrace_iterator;

public:
	/**
	 * Constructor
	 *
	 * @param code the error-code
	 * @param msg optionally, a message describing the error
	 */
	explicit Exception(ErrorCode code = E_FAILURE,const char *msg = 0) throw();

	/**
	 * Constructor with a formatted string
	 *
	 * @param code the error-code
	 * @param bufsize the size of the buffer for string to create
	 * @param msg a message describing the error
	 */
	explicit Exception(ErrorCode code,size_t bufsize,const char *fmt,...) throw();

	/**
	 * Constructor
	 *
	 * @param code the error-code
	 * @param msg a message describing the error
	 */
	explicit Exception(ErrorCode code,const String &msg) throw();

	/**
	 * Destructor
	 */
	virtual ~Exception() throw() {
	}

	/**
	 * @return the error message
	 */
	const char *msg() const {
		return _msg.str();
	}
	/**
	 * @return the error-code as a string
	 */
	const char *name() const {
		return to_string(_code);
	}
	/**
	 * @return the error-code
	 */
	ErrorCode code() const {
		return _code;
	}

	/**
	 * @return beginning of the backtrace
	 */
	backtrace_iterator backtrace_begin() const {
		return _backtrace;
	}
	/**
	 * @return end of the backtrace
	 */
	backtrace_iterator backtrace_end() const {
		return _backtrace + _count;
	}

	/**
	 * Writes this exception to the given stream. May be overwritten by subclasses.
	 *
	 * @param os the stream
	 */
	virtual void write(OStream &os) const {
		os << "Exception: " << name() << " (" << code() << ")";
		if(msg())
			os << ": " << msg();
		os << '\n';
		write_backtrace(os);
	}

protected:
	/**
	 * Convenience method to write the backtrace to the given stream
	 *
	 * @param os the stream
	 */
	void write_backtrace(OStream &os) const {
		os.writef("Backtrace:\n");
		for(backtrace_iterator it = backtrace_begin(); it != backtrace_end(); ++it)
			os.writef("\t%p\n",*it);
	}

	ErrorCode _code;
	String _msg;
	uintptr_t _backtrace[MAX_TRACE_DEPTH];
	size_t _count;
	static const char *_msgs[];
};

UtcbFrameRef &operator<<(UtcbFrameRef &uf,const Exception &e);
UtcbFrameRef &operator>>(UtcbFrameRef &uf,Exception &e);

static inline OStream &operator<<(OStream &os,const Exception &e) {
	e.write(os);
	return os;
}

}
