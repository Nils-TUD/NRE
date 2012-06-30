/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "ConsoleService.h"
#include "ConsoleSessionData.h"
#include "HostVGA.h"

using namespace nul;

ConsoleService *ConsoleService::_inst;

ConsoleService::ConsoleService(const char *name)
	: Service(name,ConsoleSessionData::portal), _screen(new HostVGA()),
	  _sess_cycler(sessions_begin(),sessions_end()) {
}

void ConsoleService::init() {
	// we want to accept two dataspaces
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LocalEc *ec = get_ec(it->log_id());
		UtcbFrameRef uf(ec->utcb());
		uf.accept_delegates(1);
	}

	// add dummy sessions for boot screen and HV screen
	add_session(new ConsoleSessionData(
			this,ConsoleSessionData::PAGE_BOOT,0,caps() + 0 * Hip::MAX_CPUS,0));
	add_session(new ConsoleSessionData(
			this,ConsoleSessionData::PAGE_HV,1,caps() + 1 * Hip::MAX_CPUS,0));
}

void ConsoleService::prev() {
	_sess_cycler.current()->to_back();
	_sess_cycler.prev()->to_front();
}

void ConsoleService::next() {
	_sess_cycler.current()->to_back();
	_sess_cycler.next()->to_front();
}

SessionData *ConsoleService::create_session(size_t id,capsel_t caps,nul::Pt::portal_func func) {
	return new ConsoleSessionData(this,ConsoleSessionData::PAGE_USER,id,caps,func);
}

void ConsoleService::created_session(size_t idx) {
	if(_sess_cycler.valid())
		_sess_cycler.current()->to_back();
	_sess_cycler.reset(sessions_begin(),iterator(this,idx),sessions_end());
	active()->to_front();
}

bool ConsoleService::handle_keyevent(const Keyboard::Packet &pk) {
	switch(pk.keycode) {
		case Keyboard::VK_LEFT:
			if(~pk.flags & Keyboard::RELEASE)
				prev();
			return true;

		case Keyboard::VK_RIGHT:
			if(~pk.flags & Keyboard::RELEASE)
				next();
			return true;

		case Keyboard::VK_UP:
			if(active() && (~pk.flags & Keyboard::RELEASE))
				active()->prev();
			return true;

		case Keyboard::VK_DOWN:
			if(active() && (~pk.flags & Keyboard::RELEASE))
				active()->next();
			return true;
	}
	return false;
}
