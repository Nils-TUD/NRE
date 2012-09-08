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

Exception::Exception(ErrorCode code,const char *msg) throw()
	: _code(code), _msg(msg), _backtrace(), _count() {
	_count = ExecEnv::collect_backtrace(&_backtrace[0],MAX_TRACE_DEPTH);
	LOG(Logging::EXCEPTIONS,Serial::get() << *this);
}

Exception::Exception(ErrorCode code,size_t bufsize,const char *fmt,...) throw()
	: _code(code), _msg(), _backtrace(), _count() {
	va_list ap;
	va_start(ap,fmt);
	_msg.vformat(bufsize,fmt,ap);
	va_end(ap);
	_count = ExecEnv::collect_backtrace(&_backtrace[0],MAX_TRACE_DEPTH);
	LOG(Logging::EXCEPTIONS,Serial::get() << *this);
}

Exception::Exception(ErrorCode code,const String &msg) throw()
	: _code(code), _msg(msg), _backtrace(), _count() {
	_count = ExecEnv::collect_backtrace(&_backtrace[0],MAX_TRACE_DEPTH);
	LOG(Logging::EXCEPTIONS,Serial::get() << *this);
}

UtcbFrameRef &operator<<(UtcbFrameRef &uf,const Exception &e) {
	uf << e.code() << e._msg;
	return uf;
}

UtcbFrameRef &operator>>(UtcbFrameRef &uf,Exception &e) {
	uf >> e._code >> e._msg;
	return uf;
}

}
