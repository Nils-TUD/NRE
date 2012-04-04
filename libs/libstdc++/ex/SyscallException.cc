/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
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
