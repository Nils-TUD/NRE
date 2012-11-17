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
 * This macro throws an exception and passes a formatted string as its message. That is, you can
 * use the stream operators to build the message. For example:
 * VTHROW(Exception, E_EXISTS, "My exception " << 1 << "," << 2 << " message");
 */
#define VTHROW(cls, errorcode, expr) {                            \
        nre::OStringStream __os;                                  \
        __os << expr;                                             \
        throw nre::cls(nre::errorcode, __os.str());               \
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
terminate_handler set_terminate(terminate_handler pHandler) throw();
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
unexpected_handler set_unexpected(unexpected_handler pHandler) throw();
/**
 * Is called by the runtime if an exception is thrown which violates the functions exception
 * specification
 */
void unexpected(void) NORETURN;
/**
 * @return true if the caught exception violates the throw specification.
 */
bool uncaught_exception() throw();

}

namespace nre {

class Exception;
class UtcbFrameRef;
static OStream &operator<<(OStream &os, const Exception &e);
UtcbFrameRef &operator<<(UtcbFrameRef &uf, const Exception &e);
UtcbFrameRef &operator>>(UtcbFrameRef &uf, Exception &e);

/**
 * The base class of all exceptions. All exceptions have an error-code, collect a backtrace and
 * have optionally a message (string).
 */
class Exception {
    friend UtcbFrameRef & operator<<(UtcbFrameRef &uf, const Exception &e);
    friend UtcbFrameRef & operator>>(UtcbFrameRef &uf, Exception &e);

    static const size_t MAX_TRACE_DEPTH = 16;

public:
    typedef const uintptr_t *backtrace_iterator;

public:
    /**
     * Constructor
     *
     * @param code the error-code
     * @param msg optionally, a message describing the error
     */
    explicit Exception(ErrorCode code = E_FAILURE, const String &msg = String()) throw();

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
        os << "Backtrace:\n";
        for(backtrace_iterator it = backtrace_begin(); it != backtrace_end(); ++it)
            os << "\t" << fmt(*it, "p") << "\n";
    }

    ErrorCode _code;
    String _msg;
    uintptr_t _backtrace[MAX_TRACE_DEPTH];
    size_t _count;
    static const char *_msgs[];
};

UtcbFrameRef &operator<<(UtcbFrameRef &uf, const Exception &e);
UtcbFrameRef &operator>>(UtcbFrameRef &uf, Exception &e);

static inline OStream &operator<<(OStream &os, const Exception &e) {
    e.write(os);
    return os;
}

}
