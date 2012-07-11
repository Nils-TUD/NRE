/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ipc/Producer.h>
#include <ipc/Consumer.h>
#include <kobj/Sc.h>
#include <kobj/GlobalThread.h>
#include <mem/DataSpace.h>

#include "Screen.h"

class ConsoleSessionData;

class ViewSwitcher {
	enum {
		DS_SIZE			= nre::ExecEnv::PAGE_SIZE * Screen::PAGES,
		COLOR			= 0x1F,
		SWITCH_TIME		= 1000,	// ms
		REFRESH_DELAY	= 25	// ms
	};

	struct SwitchCommand {
		size_t oldsessid;
		size_t sessid;
	};

public:
	explicit ViewSwitcher();

	void start() {
		_sc.start();
	}

	void switch_to(ConsoleSessionData *from,ConsoleSessionData *to);

private:
	static void switch_thread(void*);

	nre::DataSpace _ds;
	nre::Producer<SwitchCommand> _prod;
	nre::Consumer<SwitchCommand> _cons;
	nre::GlobalThread _ec;
	nre::Sc _sc;
	static char _backup[];
	static char _buffer[];
};
