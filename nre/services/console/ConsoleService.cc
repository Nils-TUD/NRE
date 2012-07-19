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

#include "ConsoleService.h"
#include "ConsoleSessionData.h"
#include "HostVGA.h"

using namespace nre;

ConsoleService *ConsoleService::_inst;

ConsoleService::ConsoleService(const char *name)
	: Service(name,ConsoleSessionData::portal), _con("reboot"), _reboot(_con),
	  _screen(new HostVGA()), _sess_cycler(sessions_begin(),sessions_end()), _switcher() {
}

void ConsoleService::init() {
	// we want to accept two dataspaces
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LocalThread *ec = get_ec(it->log_id());
		UtcbFrameRef uf(ec->utcb());
		uf.accept_delegates(1);
	}

	// add dummy session for boot screen and HV screen
	ConsoleSessionData *sess = new ConsoleSessionData(this,0,caps(),0);
	DataSpace *ds = new DataSpace(ExecEnv::PAGE_SIZE * Screen::PAGES,DataSpaceDesc::ANONYMOUS,
			DataSpaceDesc::RW);
	// copy screen to buffer
	memcpy(reinterpret_cast<void*>(ds->virt()),reinterpret_cast<void*>(_screen->mem().virt()),
			ExecEnv::PAGE_SIZE * Screen::PAGES);
	sess->create(0,ds);
	add_session(sess);
	_switcher.start();
}

void ConsoleService::prev() {
	ConsoleSessionData *old = active();
	iterator it = _sess_cycler.prev();
	_switcher.switch_to(old,&*it);
}

void ConsoleService::next() {
	ConsoleSessionData *old = active();
	iterator it = _sess_cycler.next();
	_switcher.switch_to(old,&*it);
}

SessionData *ConsoleService::create_session(size_t id,capsel_t caps,Pt::portal_func func) {
	return new ConsoleSessionData(this,id,caps,func);
}

void ConsoleService::created_session(size_t idx) {
	ConsoleSessionData *old = active();
	iterator it = iterator(this,idx);
	_sess_cycler.reset(sessions_begin(),it,sessions_end());
	_switcher.switch_to(old,&*it);
}

bool ConsoleService::handle_keyevent(const Keyboard::Packet &pk) {
	if((~pk.flags & Keyboard::LWIN) || (~pk.flags & Keyboard::RELEASE))
		return false;
	switch(pk.keycode) {
		case Keyboard::VK_END:
			_reboot.reboot();
			return true;

		case Keyboard::VK_LEFT:
			prev();
			return true;

		case Keyboard::VK_RIGHT:
			next();
			return true;

		case Keyboard::VK_UP:
			if(active())
				active()->prev();
			return true;

		case Keyboard::VK_DOWN:
			if(active())
				active()->next();
			return true;
	}
	return false;
}
