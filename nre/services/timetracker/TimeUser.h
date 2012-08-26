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

#include <util/SList.h>
#include <String.h>

class TimeUser : public nre::SListItem {
public:
	explicit TimeUser(const nre::String &name,cpu_t cpu,capsel_t sc)
		: nre::SListItem(), _name(name), _cpu(cpu), _sc(sc) {
	}

	const nre::String &name() const {
		return _name;
	}
	cpu_t cpu() const {
		return _cpu;
	}
	capsel_t cap() const {
		return _sc;
	}

private:
	nre::String _name;
	cpu_t _cpu;
	capsel_t _sc;
};
