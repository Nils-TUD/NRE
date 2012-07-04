/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <service/Service.h>
#include <service/Connection.h>
#include <dev/Keyboard.h>
#include <dev/Reboot.h>
#include <util/Cycler.h>

#include "Screen.h"
#include "ViewSwitcher.h"

class ConsoleSessionData;

class ConsoleService : public nul::Service {
public:
	typedef nul::SessionIterator<ConsoleSessionData> iterator;

	static ConsoleService *get() {
		return _inst;
	}
	static ConsoleService *create(const char *name) {
		return _inst = new ConsoleService(name);
	}

	void init();

	ConsoleSessionData *active() {
		iterator it = _sess_cycler.current();
		return it != sessions_end() ? &*it : 0;
	}

	void prev();
	void next();

	iterator sessions_begin() {
		return Service::sessions_begin<ConsoleSessionData>();
	}
	iterator sessions_end() {
		return Service::sessions_end<ConsoleSessionData>();
	}

	ViewSwitcher &switcher() {
		return _switcher;
	}
	Screen *screen() {
		return _screen;
	}
	bool handle_keyevent(const nul::Keyboard::Packet &pk);

private:
	ConsoleService(const char *name);

	virtual nul::SessionData *create_session(size_t id,capsel_t caps,nul::Pt::portal_func func);
	virtual void created_session(size_t idx);

	nul::Connection _con;
	nul::RebootSession _reboot;
	Screen *_screen;
	nul::Cycler<iterator> _sess_cycler;
	ViewSwitcher _switcher;
	static ConsoleService *_inst;
};
