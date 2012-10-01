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

/**
 * Provides the portal to open/close sessions for a specific CPU. Additionally, it hosts the thread
 * to do so and the thread for the actual service-portal.
 */
class ServiceCPUHandler {
public:
	/**
	 * Constructor. Creates the threads and portals
	 *
	 * @param s the service
	 * @param pt the portal-selector
	 * @param cpu the CPU to run on
	 */
	explicit ServiceCPUHandler(Service* s,capsel_t pt,cpu_t cpu);

	/**
	 * @return the CPU
	 */
	cpu_t cpu() const {
		return _session_ec->cpu();
	}
	/**
	 * @return the thread that is used for the service-portal
	 */
	LocalThread &thread() {
		return *_session_ec;
	}

private:
	ServiceCPUHandler(const ServiceCPUHandler&);
	ServiceCPUHandler& operator=(const ServiceCPUHandler&);

	PORTAL static void portal(capsel_t pid);

	Service *_s;
	LocalThread *_session_ec;
	LocalThread *_service_ec;
	Pt _pt;
	UserSm _sm;
};

}
