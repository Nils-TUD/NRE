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

#include <mem/DataSpace.h>
#include <subsystem/ChildManager.h>

class VMConfig {
public:
	explicit VMConfig(uintptr_t phys,size_t size,const char *cmdline)
		: _ds(size,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::R,phys), _name(cmdline) {
	}

	const char *name() const {
		return _name;
	}
	void start(nre::ChildManager *cm);

private:
	static const char *get_cmdline(const char *begin);
	static nre::Hip::mem_iterator get_module(const char *name,size_t namelen);
	static const char *get_name(const char *str,size_t &len);

	nre::DataSpace _ds;
	const char *_name;
};
