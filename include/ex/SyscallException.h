/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ex/Exception.h>
#include <Errors.h>

class SyscallException : public Exception {
public:
	SyscallException(ErrorCode code) throw() : Exception(), _code(code) {
	}
	virtual ~SyscallException() throw() {
	}

	ErrorCode error_code() const throw() {
		return _code;
	}
	const char *error_msg() const throw();

	virtual void print(Format& fmt) const {
		fmt.print("Systemcall failed: %s (%d)\n",error_msg(),error_code());
		print_backtrace(fmt);
	}

private:
	ErrorCode _code;
	static const char *_msgs[];
};
