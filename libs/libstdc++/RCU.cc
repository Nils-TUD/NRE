/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <arch/Startup.h>
#include <RCU.h>

namespace nre {
	class Init {
		Init() {
			RCU::announce(Thread::current());
		}
		static Init init;
	};

	uint32_t *RCU::_versions = 0;
	size_t RCU::_versions_count = 0;
	Thread *RCU::_ecs = 0;
	size_t RCU::_ec_count = 0;
	RCUObject *RCU::_objs = 0;
	RCULock RCU::_lock;
	UserSm RCU::_sm INIT_PRIO_RCU;
	Init Init::init INIT_PRIO_RCU;
}
