/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <Errors.h>

namespace nul {

static const char *msgs[] = {
	/* E_SUCCESS */ 		"Successful Operation",
	/* E_TIMEOUT */ 		"Communication Timeout",
	/* E_ABORT */			"Communication Abort",
	/* E_SYS */				"Invalid Hypercall",
	/* E_CAP */				"Invalid Capability",
	/* E_PAR */				"Invalid Parameter",
	/* E_FTR */				"Invalid Feature",
	/* E_CPU */				"Invalid CPU Number",
	/* E_DEV */				"Invalid Device ID",

	/* E_ASSERT */			"Assert failed",
	/* E_NO_CAP_SELS */		"No free capability selectors",
	/* E_UTCB_UNTYPED */	"No more untyped items in UTCB",
	/* E_UTCB_TYPED */		"No more typed items in UTCB",
	/* E_CAPACITY */		"Capacity exceeded",
	/* E_NOT_FOUND */		"Object not found",
	/* E_EXISTS */			"Object does already exist",
	/* E_ELF_INVALID */		"Invalid ELF file",
	/* E_ELF_SIG */			"Invalid ELF signature",
};

const char *to_string(ErrorCode code) {
	if(code < sizeof(msgs) / sizeof(msgs[0]))
		return msgs[code];
	return "Unknown error";
}

}
