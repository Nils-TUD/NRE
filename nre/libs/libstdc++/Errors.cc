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

#include <arch/Defines.h>
#include <Errors.h>

namespace nre {

static const char *msgs[] = {
	/* E_SUCCESS */       "Successful Operation",
	/* E_TIMEOUT */       "Communication Timeout",
	/* E_ABORT */         "Communication Abort",
	/* E_SYS */           "Invalid Hypercall",
	/* E_CAP */           "Invalid Capability",
	/* E_PAR */           "Invalid Parameter",
	/* E_FTR */           "Invalid Feature",
	/* E_CPU */           "Invalid CPU Number",
	/* E_DEV */           "Invalid Device ID",
	/* E_ASSERT */        "Assert failed",
	/* E_NO_CAP_SELS */   "No free capability selectors",
	/* E_UTCB_UNTYPED */  "No more untyped items in UTCB",
	/* E_UTCB_TYPED */    "No more typed items in UTCB",
	/* E_CAPACITY */      "Capacity exceeded",
	/* E_NOT_FOUND */     "Object not found",
	/* E_EXISTS */        "Object does already exist",
	/* E_ELF_INVALID */   "Invalid ELF file",
	/* E_ELF_SIG */       "Invalid ELF signature",
	/* E_ARGS_INVALID */  "Invalid arguments",
	/* E_FAILURE */       "Failure",
};

const char *to_string(ErrorCode code) {
	if(code < ARRAY_SIZE(msgs))
		return msgs[code];
	return "Unknown error";
}

}
