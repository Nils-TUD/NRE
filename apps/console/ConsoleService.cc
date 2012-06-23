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
CPU::iterator ConsoleSessionData::_cpu_it;

SessionData *ConsoleService::create_session(size_t id,capsel_t caps,nul::Pt::portal_func func) {
	return new ConsoleSessionData(this,id,caps,func);
}

bool ConsoleService::handle_keyevent(const Keyboard::Packet &pk) {
	if(_active == sessions_end())
		_active = sessions_begin();
	if(_active == sessions_end())
		return false;

	switch(pk.keycode) {
		case Keyboard::VK_LEFT:
			if(~pk.flags & Keyboard::RELEASE) {
				if(_active == sessions_begin())
					_active = sessions_end();
				--_active;
				_active->switch_to(_active->view());
			}
			return true;

		case Keyboard::VK_RIGHT:
			if(~pk.flags & Keyboard::RELEASE) {
				++_active;
				if(_active == sessions_end())
					_active = sessions_begin();
				_active->switch_to(_active->view());
			}
			return true;

		case Keyboard::VK_UP:
			if(~pk.flags & Keyboard::RELEASE)
				_active->switch_to((_active->view() - 1) % Screen::VIEW_COUNT);
			return true;

		case Keyboard::VK_DOWN:
			if(~pk.flags & Keyboard::RELEASE)
				_active->switch_to((_active->view() + 1) % Screen::VIEW_COUNT);
			return true;
	}
	return false;
}
