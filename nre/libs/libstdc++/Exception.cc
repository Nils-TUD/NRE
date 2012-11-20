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

#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <Logging.h>

namespace nre {

Exception::Exception(ErrorCode code, const String &msg) throw()
    : _code(code), _msg(msg), _backtrace(), _count() {
    _count = ExecEnv::collect_backtrace(&_backtrace[0], MAX_TRACE_DEPTH);
    LOG(EXCEPTIONS, *this);
}

void Exception::write(OStream &os) const {
    os << "Exception: " << name() << " (" << code() << ")";
    if(msg())
        os << ": " << msg();
    os << '\n';
    write_backtrace(os);
}

void Exception::write_backtrace(OStream &os) const {
    os << "Backtrace:\n";
    for(auto it = backtrace_begin(); it != backtrace_end(); ++it)
        os << "\t" << fmt(*it, "p") << "\n";
}

OStream &operator<<(OStream &os, const Exception &e) {
    e.write(os);
    return os;
}

UtcbFrameRef & operator<<(UtcbFrameRef &uf, const Exception &e) {
    uf << e.code() << e._msg;
    return uf;
}

UtcbFrameRef & operator>>(UtcbFrameRef &uf, Exception &e) {
    uf >> e._code >> e._msg;
    return uf;
}

}
