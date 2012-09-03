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

#include <arch/SpinLock.h>
#include <arch/Types.h>
#include <Exception.h>
#include <util/ScopedLock.h>
#include <Hip.h>

namespace nre {

class CapException : public Exception {
public:
	DEFINE_EXCONSTRS(CapException)
};

/**
 * The capability selector space contains the selectors for your capabilites. This class manages
 * the selectors. That is, you can allocate and release selectors.
 */
class CapSelSpace {
public:
	/**
	 * Selectors with special meaning
	 */
	enum Caps {
		// the exception portals
		EV_DIVIDE		= 0x0,
		EV_DEBUG		= 0x1,
		EV_BREAKPOINT	= 0x3,
		EV_OVERFLOW		= 0x4,
		EV_BOUNDRANGE	= 0x5,
		EV_UNDEFOP		= 0x6,
		EV_NOMATHPROC	= 0x7,
		EV_DBLFAULT		= 0x8,
		EV_TSS			= 0xA,
		EV_INVSEG		= 0xB,
		EV_STACK		= 0xC,
		EV_GENPROT		= 0xD,
		EV_PAGEFAULT	= 0xE,
		EV_MATHFAULT	= 0x10,
		EV_ALIGNCHK		= 0x11,
		EV_MACHCHK		= 0x12,
		EV_SIMD			= 0x13,
		EV_STARTUP		= 0x1E,
		EV_RECALL		= 0x1F,

		// our own Pd, Ec and Sc are aways at these offsets
		INIT_PD			= 0x20,
		INIT_EC			= 0x21,
		INIT_SC			= 0x22,

		// service portals
		SRV_INIT		= 0x23,		// get initial caps
		SRV_SERVICE		= 0x24,		// service portal
		SRV_IO			= 0x25,		// io ports portal
		SRV_GSI			= 0x26,		// global system interrupt portal
		SRV_DS			= 0x27,		// dataspace portal
		SRV_SC			= 0x28,		// Sc portal
	};

	/**
	 * @return the instance of this class
	 */
	static CapSelSpace& get() {
		return _inst;
	}

	/**
	 * Allocates <count> selectors with alignment <align>.
	 *
	 * @param count the number of selectors to allocate (default = 1)
	 * @param align the alignment of the selectors (default = 1). has to be a power of 2!
	 */
	capsel_t allocate(uint count = 1,uint align = 1) {
		ScopedLock<SpinLock> lock(&_lck);
		capsel_t res = (_off + align - 1) & ~(align - 1);
		if(res + count < res || res + count > Hip::get().cfg_cap)
			throw CapException(E_NO_CAP_SELS);
		_off = res + count;
		return res;
	}
	/**
	 * Free's the selectors <base>...<base>+<count>-1
	 *
	 * @param base the base of the selectors
	 * @param count the number (default = 1)
	 */
	void free(UNUSED capsel_t base,UNUSED uint count = 1) {
		// TODO implement me
	}

private:
	explicit CapSelSpace() : _lck(), _off(Hip::get().object_caps()) {
	}

	CapSelSpace(const CapSelSpace&);
	CapSelSpace& operator=(const CapSelSpace&);

	static CapSelSpace _inst;
	SpinLock _lck;
	capsel_t _off;
};

}
