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

#include <Hip.h>

namespace nre {

cpu_t Hip::cpu_phys_to_log(cpu_t cpu) const {
	cpu_t phys = 0;
	for(cpu_iterator it = cpu_begin(); it != cpu_end(); ++it, ++phys) {
		if(cpu == it->id())
			break;
	}
	return phys;
}

}
