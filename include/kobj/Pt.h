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

#include <arch/ExecEnv.h>
#include <kobj/ObjCap.h>
#include <kobj/LocalEc.h>
#include <utcb/UtcbFrame.h>
#include <Compiler.h>
#include <Syscalls.h>

namespace nul {

/**
 * Represents a portal. A portal is always bound to a LocalEc.
 */
class Pt : public ObjCap {
public:
	typedef ExecEnv::portal_func portal_func;

	/**
	 * Attaches a portal object to the given portal-capability-selector. The destructor will neither
	 * free the selector nor the capability.
	 *
	 * @param pt the capability-selector for the portal
	 */
	explicit Pt(capsel_t pt) : ObjCap(pt,KEEP_CAP_BIT | KEEP_SEL_BIT) {
	}

	/**
	 * Creates a portal for <func> at selector <pt> that is bound to the given Ec. The destructor
	 * will not free the selector, but only the capability.
	 *
	 * @param ec the LocalEc to bind the portal to
	 * @param pt the capability selector to use
	 * @param func the portal function
	 * @param mtd the message-transfer descriptor to describe what information should the
	 * 	kernel pass to the portal
	 */
	explicit Pt(LocalEc *ec,capsel_t pt,portal_func func,Mtd mtd = Mtd()) : ObjCap(pt,KEEP_SEL_BIT) {
		Syscalls::create_pt(pt,ec->sel(),reinterpret_cast<uintptr_t>(func),mtd,Pd::current()->sel());
	}

	/**
	 * Creates a portal for <func> that is bound to the given Ec.
	 *
	 * @param ec the LocalEc to bind the portal to
	 * @param func the portal function
	 * @param mtd the message-transfer descriptor to describe what information should the
	 * 	kernel pass to the portal
	 */
	explicit Pt(LocalEc *ec,portal_func func,Mtd mtd = Mtd()) : ObjCap() {
		ScopedCapSels pt;
		Syscalls::create_pt(pt.get(),ec->sel(),reinterpret_cast<uintptr_t>(func),mtd,Pd::current()->sel());
		sel(pt.release());
	}

	/**
	 * Calls this portal with given UtcbFrame. The state of the UtcbFrame is reset afterwards, so
	 * that you can iterate over the typed and untyped item again from the beginning.
	 * Note: although you can specify the UtcbFrame, you can't really choose it. That is, the kernel
	 * will always use the Utcb that belongs to your Ec. The parameter is rather passed symbolically
	 * to make clear that the UtcbFrame you're working with is changed by the call.
	 *
	 * @param uf the UtcbFrame
	 */
	void call(UtcbFrame &uf) {
		Syscalls::call(sel());
		uf._upos = 0;
		uf._tpos = 0;
	}
};

}
