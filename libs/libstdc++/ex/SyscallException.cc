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

#include <ex/SyscallException.h>

const char *SyscallException::_msgs[] = {
	/* SUCCESS */ "Successful Operation",
	/* COM_TIM */ "Communication Timeout",
	/* COM_ABT */ "Communication Abort",
	/* BAD_HYP */ "Invalid Hypercall",
	/* BAD_CAP */ "Invalid Capability",
	/* BAD_PAR */ "Invalid Parameter",
	/* BAD_FTR */ "Invalid Feature",
	/* BAD_CPU */ "Invalid CPU Number",
	/* BAD_DEV */ "Invalid Device ID",
};

const char *SyscallException::error_msg() const throw() {
	if(_code < sizeof(_msgs) / sizeof(_msgs[0]))
		return _msgs[_code];
	return "Unknown error";
}
