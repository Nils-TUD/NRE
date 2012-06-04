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

#include <Exception.h>

namespace nul {

class SyscallException : public Exception {
public:
	explicit SyscallException(ErrorCode code) throw() : Exception(code) {
	}

	virtual void write(OStream& os) const {
		os.writef("Systemcall failed: %s (%d)\n",name(),code());
		write_backtrace(os);
	}
};

}

#ifdef __i386__
#include <arch/x86_32/SyscallABI.h>
#elif defined __x86_64__
#include <arch/x86_64/SyscallABI.h>
#else
#error "Unsupported architecture"
#endif
