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

ConsoleService::ConsoleService(const char *name)
	: Service(name,CPUSet(CPUSet::ALL),ConsoleSessionData::portal), _rbcon("reboot"), _reboot(_rbcon),
	  _screen(new HostVGA()), _cons(), _concyc(), _switcher(this) {
	// we want to accept two dataspaces
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LocalThread *t = get_thread(it->log_id());
		t->set_tls<ConsoleService*>(Thread::TLS_PARAM,this);
		UtcbFrameRef uf(t->utcb());
		uf.accept_delegates(1);
	}

	// add dummy session for boot screen and HV screen
	create_dummy(0,String("Bootloader"));
	create_dummy(1,String("Hypervisor"));
	_switcher.start();
}

void ConsoleService::create_dummy(uint page,const String &title) {
	ConsoleSessionData *sess = static_cast<ConsoleSessionData*>(new_session());
	sess->set_page(page);
	DataSpace *ds = new DataSpace(ExecEnv::PAGE_SIZE * Screen::PAGES,DataSpaceDesc::ANONYMOUS,
			DataSpaceDesc::RW);
	memset(reinterpret_cast<void*>(ds->virt()),0,ExecEnv::PAGE_SIZE * Screen::PAGES);
	memcpy(reinterpret_cast<void*>(ds->virt() + sess->offset()),
			reinterpret_cast<void*>(_screen->mem().virt() + sess->offset()),
			ExecEnv::PAGE_SIZE);
	sess->create(0,ds,0,title);
}

void ConsoleService::up() {
	ScopedLock<UserSm> guard(&_sm);
	ConsoleSessionData *old = active();
	iterator it = _concyc[_console]->prev();
	_switcher.switch_to(old,&*it);
}

void ConsoleService::down() {
	ScopedLock<UserSm> guard(&_sm);
	ConsoleSessionData *old = active();
	iterator it = _concyc[_console]->next();
	_switcher.switch_to(old,&*it);
}

void ConsoleService::left() {
	ScopedLock<UserSm> guard(&_sm);
	left_unlocked();
}

void ConsoleService::left_unlocked() {
	ConsoleSessionData *old = active();
	do {
		_console = (_console - 1) % Console::SUBCONS;
	}
	while(_cons[_console] == 0);
	iterator it = _concyc[_console]->current();
	_switcher.switch_to(old,&*it);
}

void ConsoleService::right() {
	ScopedLock<UserSm> guard(&_sm);
	ConsoleSessionData *old = active();
	do {
		_console = (_console + 1) % Console::SUBCONS;
	}
	while(_cons[_console] == 0);
	iterator it = _concyc[_console]->current();
	_switcher.switch_to(old,&*it);
}

ServiceSession *ConsoleService::create_session(size_t id,capsel_t caps,Pt::portal_func func) {
	return new ConsoleSessionData(this,id,caps,func);
}

void ConsoleService::remove(ConsoleSessionData *sess) {
	ScopedLock<UserSm> guard(&_sm);
	size_t con = sess->console();
	_cons[con]->remove(sess);
	if(_cons[con]->length() == 0) {
		delete _cons[con];
		delete _concyc[con];
		_cons[con] = 0;
		_concyc[con] = 0;
		if(_console == con)
			left_unlocked();
	}
	else {
		iterator it = _cons[con]->begin();
		_concyc[con]->reset(it,it,_cons[con]->end());
		if(_console == con)
			_switcher.switch_to(0,&*it);
	}
}

void ConsoleService::session_ready(ConsoleSessionData *sess) {
	ScopedLock<UserSm> guard(&_sm);
	ConsoleSessionData *old = active();
	_console = sess->console();
	if(_cons[_console] == 0) {
		_cons[_console] = new nre::DList<ConsoleSessionData>();
		_cons[_console]->append(sess);
		_concyc[_console] = new Cycler<iterator>(
				_cons[_console]->begin(),_cons[_console]->end());
	}
	else {
		iterator it = _cons[_console]->append(sess);
		_concyc[_console]->reset(
				_cons[_console]->begin(),it,_cons[_console]->end());
	}
	_switcher.switch_to(old,sess);
}

bool ConsoleService::handle_keyevent(const Keyboard::Packet &pk) {
	if((~pk.flags & Keyboard::LCTRL) || (~pk.flags & Keyboard::RELEASE))
		return false;
	switch(pk.keycode) {
		case Keyboard::VK_END:
			_reboot.reboot();
			return true;

		case Keyboard::VK_LEFT:
			left();
			return true;

		case Keyboard::VK_RIGHT:
			right();
			return true;

		case Keyboard::VK_UP:
			up();
			return true;

		case Keyboard::VK_DOWN:
			down();
			return true;
	}
	return false;
}
