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

#include <kobj/LocalThread.h>
#include <kobj/Pt.h>
#include <kobj/UserSm.h>
#include <CPU.h>

namespace nre {

class Service;

class ServiceInstance {
public:
	explicit ServiceInstance(Service* s,capsel_t pt,cpu_t cpu);

	cpu_t cpu() const {
		return _session_ec.cpu();
	}
	LocalThread &ec() {
		return _session_ec;
	}

private:
	ServiceInstance(const ServiceInstance&);
	ServiceInstance& operator=(const ServiceInstance&);

	PORTAL static void portal(capsel_t pid);

	Service *_s;
	LocalThread _session_ec;
	LocalThread _service_ec;
	Pt _pt;
	UserSm _sm;
};

}
