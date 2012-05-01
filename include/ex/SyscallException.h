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

#include <ex/Exception.h>
#include <Errors.h>

namespace nul {

class SyscallException : public Exception {
public:
	explicit SyscallException(ErrorCode code) throw() : Exception(), _code(code) {
	}
	virtual ~SyscallException() throw() {
	}

	ErrorCode error_code() const throw() {
		return _code;
	}
	const char *error_msg() const throw();

	virtual void write(OStream& os) const {
		os.writef("Systemcall failed: %s (%d)\n",error_msg(),error_code());
		write_backtrace(os);
	}

private:
	ErrorCode _code;
	static const char *_msgs[];
};

}
