/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <service/Producer.h>
#include <service/Consumer.h>
#include <kobj/Sc.h>
#include <kobj/GlobalEc.h>
#include <mem/DataSpace.h>

class ConsoleSessionData;

class ViewSwitcher {
	enum {
		DS_SIZE	= nul::ExecEnv::PAGE_SIZE
	};

	struct SwitchCommand {
		size_t sessid;
	};

public:
	explicit ViewSwitcher();

	void switch_to(ConsoleSessionData *sess);

private:
	static void switch_thread(void*);

	nul::DataSpace _ds;
	nul::Producer<SwitchCommand> _prod;
	nul::Consumer<SwitchCommand> _cons;
	nul::GlobalEc _ec;
	nul::Sc _sc;
};
