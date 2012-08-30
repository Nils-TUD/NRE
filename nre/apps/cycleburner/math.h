// -*- Mode: C++ -*-
/** @file
 * Math helpers
 *
 * Copyright (C) 2009, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#define M_PI	3.14159265358979323846

// somehow gcc is unable to return a float with SSE disabled on x86_64 (gcc bug?). it works when
// passing it as reference and changing it.
static inline void fsin(float &f) {
	asm ("fsin\n" : "+t" (f));
}

static inline void fsqrt(float &f) {
	asm ("fsqrt\n" : "+t" (f));
}

/* EOF */
