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
#include <dev/Screen.h>
#include <dev/Keyboard.h>

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

	iterator sessions_begin() {
		return Service::sessions_begin<ConsoleSessionData>();
	}
	iterator sessions_end() {
		return Service::sessions_end<ConsoleSessionData>();
	}

	ConsoleSessionData *active() {
		return _active != sessions_end() ? &*_active : 0;
	}
	nul::ScreenSession &screen() {
		return _sess;
	}
	bool handle_keyevent(const nul::Keyboard::Packet &pk);

private:
	ConsoleService(const char *name)
		: Service(name), _screen("screen"), _sess(_screen), _active(sessions_begin()) {
	}

	virtual nul::SessionData *create_session(size_t id,capsel_t caps,nul::Pt::portal_func func);

	nul::Connection _screen;
	nul::ScreenSession _sess;
	iterator _active;
	static ConsoleService *_inst;
};
