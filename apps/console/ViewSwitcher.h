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
#include <kobj/GlobalThread.h>
#include <mem/DataSpace.h>

class ConsoleSessionData;
class ConsoleSessionView;

class ViewSwitcher {
	enum {
		DS_SIZE			= nul::ExecEnv::PAGE_SIZE,
		COLOR			= 0x1F,
		SWITCH_TIME		= 1000,	// ms
		REFRESH_DELAY	= 25	// ms
	};

	struct SwitchCommand {
		size_t oldsessid;
		size_t sessid;
		size_t oldviewid;
		size_t viewid;
	};

public:
	explicit ViewSwitcher();

	void start() {
		_sc.start();
	}

	void switch_to(ConsoleSessionData *from,ConsoleSessionData *to);
	void switch_to(ConsoleSessionView *from,ConsoleSessionView *to);

private:
	static void switch_thread(void*);

	nul::DataSpace _ds;
	nul::Producer<SwitchCommand> _prod;
	nul::Consumer<SwitchCommand> _cons;
	nul::GlobalThread _ec;
	nul::Sc _sc;
	static char _backup[];
	static char _buffer[];
};
