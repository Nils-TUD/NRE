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
#include <util/DList.h>

#include "Screen.h"
#include "ViewSwitcher.h"

class ConsoleSessionData;

class ConsoleService : public nre::Service {
public:
	typedef nre::DList<ConsoleSessionData>::iterator iterator;

	ConsoleService(const char *name);

	ConsoleSessionData *active() {
		if(_concyc[_console] == 0)
			return 0;
		iterator it = _concyc[_console]->current();
		return it != _cons[_console]->end() ? &*it : 0;
	}

	void remove(ConsoleSessionData *sess);
	void up();
	void down();
	void left();
	void left_unlocked();
	void right();

	ViewSwitcher &switcher() {
		return _switcher;
	}
	Screen *screen() {
		return _screen;
	}
	void session_ready(ConsoleSessionData *sess);
	bool handle_keyevent(const nre::Keyboard::Packet &pk);

private:
	virtual nre::ServiceSession *create_session(size_t id,capsel_t caps,nre::Pt::portal_func func);
	void create_dummy(uint page,const nre::String &title);

	nre::Connection _rbcon;
	nre::RebootSession _reboot;
	Screen *_screen;
	size_t _console;
	nre::DList<ConsoleSessionData> *_cons[nre::Console::SUBCONS];
	nre::Cycler<iterator> *_concyc[nre::Console::SUBCONS];
	nre::UserSm _sm;
	ViewSwitcher _switcher;
};
