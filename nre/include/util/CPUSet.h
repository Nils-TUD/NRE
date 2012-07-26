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

#include <util/BitField.h>
#include <Hip.h>

namespace nre {

class CPUSet {
public:
	enum Preset {
		NONE,
		ALL
	};

	explicit CPUSet(Preset preset = ALL) : _cpus() {
		if(preset == ALL)
			_cpus.set_all();
	}

	const BitField<Hip::MAX_CPUS> &get() const {
		return _cpus;
	}
	void set(cpu_t cpu) {
		_cpus.set(cpu);
	}

private:
	BitField<Hip::MAX_CPUS> _cpus;
};

}
