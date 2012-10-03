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

#include <ipc/Service.h>
#include <subsystem/ChildManager.h>

/**
 * The sysinfo-service is intended to allow applications to display information about the running
 * system to the user. At the moment, you can get information about the existing Scs, and the
 * child tasks of root with the memory usage and some other things.
 */
class SysInfoService : public nre::Service {
public:
	SysInfoService(nre::ChildManager *cm)
		: nre::Service("sysinfo",nre::CPUSet(nre::CPUSet::ALL),portal), _cm(cm) {
		for(nre::CPU::iterator it = nre::CPU::begin(); it != nre::CPU::end(); ++it) {
			nre::LocalThread *ec = get_thread(it->log_id());
			ec->set_tls<SysInfoService*>(nre::Thread::TLS_PARAM,this);
		}
	}

private:
	const char *get_root_info(size_t &virt,size_t &phys,size_t &threads);
	PORTAL static void portal(capsel_t pid);

	nre::ChildManager *_cm;
};
