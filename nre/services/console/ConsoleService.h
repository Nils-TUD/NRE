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

#include <ipc/Service.h>
#include <ipc/Connection.h>
#include <services/Keyboard.h>
#include <services/Reboot.h>
#include <util/Cycler.h>

#include "Screen.h"
#include "ViewSwitcher.h"

class ConsoleSessionData;

class ConsoleService : public nre::Service {
public:
	typedef nre::SessionIterator<ConsoleSessionData> iterator;

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
	bool handle_keyevent(const nre::Keyboard::Packet &pk);

private:
	ConsoleService(const char *name);

	virtual nre::SessionData *create_session(size_t id,capsel_t caps,nre::Pt::portal_func func);
	virtual void created_session(size_t idx);

	nre::Connection _con;
	nre::RebootSession _reboot;
	Screen *_screen;
	nre::Cycler<iterator> _sess_cycler;
	ViewSwitcher _switcher;
	static ConsoleService *_inst;
};
