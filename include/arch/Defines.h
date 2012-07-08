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

#ifdef __i386__
#include <arch/x86_32/Defines.h>
#elif defined __x86_64__
#include <arch/x86_64/Defines.h>
#else
#error "Unsupported architecture"
#endif

#define STRING(x)		#x
#define EXPAND(x)		STRING(x)
#define ARRAY_SIZE(X)	(sizeof((X)) / sizeof((X)[0]))
