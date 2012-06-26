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

using namespace nul;

ConsoleService *ConsoleService::_inst;

ConsoleService::ConsoleService(const char *name)
	: Service(name,ConsoleSessionData::portal), _screen("screen"), _sess(_screen),
	  _sess_cycler(sessions_begin(),sessions_end()) {
}

void ConsoleService::init() {
	// we want to accept two dataspaces
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LocalEc *ec = get_ec(it->id);
		UtcbFrameRef uf(ec->utcb());
		uf.accept_delegates(1);
	}

	// add dummy sessions for boot screen and HV screen
	add_session(new ConsoleSessionData(
			this,ConsoleSessionData::PAGE_BOOT,0,caps() + 0 * Hip::MAX_CPUS,0));
	add_session(new ConsoleSessionData(
			this,ConsoleSessionData::PAGE_HV,1,caps() + 1 * Hip::MAX_CPUS,0));
}

SessionData *ConsoleService::create_session(size_t id,capsel_t caps,nul::Pt::portal_func func) {
	return new ConsoleSessionData(this,ConsoleSessionData::PAGE_USER,id,caps,func);
}

void ConsoleService::created_session(size_t idx) {
	_sess_cycler.reset(sessions_begin(),iterator(this,idx),sessions_end());
	active()->repaint();
}

bool ConsoleService::handle_keyevent(const Keyboard::Packet &pk) {
	bool res = false;
	switch(pk.keycode) {
		case Keyboard::VK_LEFT:
			if(~pk.flags & Keyboard::RELEASE)
				prev();
			res = true;
			break;

		case Keyboard::VK_RIGHT:
			if(~pk.flags & Keyboard::RELEASE)
				next();
			res = true;
			break;

		case Keyboard::VK_UP:
			if(active() && (~pk.flags & Keyboard::RELEASE))
				active()->prev();
			res = true;
			break;

		case Keyboard::VK_DOWN:
			if(active() && (~pk.flags & Keyboard::RELEASE))
				active()->next();
			res = true;
			break;
	}
	if(res && active())
		active()->repaint();
	return res;
}
