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

namespace nre {

enum ErrorCode {
	// NOVA error codes
	E_SUCCESS = 0,
	E_TIMEOUT,
	E_ABORT,
	E_SYS,
	E_CAP,
	E_PAR,
	E_FTR,
	E_CPU,
	E_DEV,

	// NUL error codes
	E_ASSERT,
	E_NO_CAP_SELS,
	E_UTCB_UNTYPED,
	E_UTCB_TYPED,
	E_CAPACITY,
	E_NOT_FOUND,
	E_EXISTS,
	E_ELF_INVALID,
	E_ELF_SIG,
	E_ARGS_INVALID,
	E_FAILURE,
};

const char *to_string(ErrorCode code);

}
