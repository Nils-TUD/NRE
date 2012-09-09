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

#include <kobj/UserSm.h>
#include <services/Console.h>
#include <util/DList.h>

#include "ConsoleService.h"

class ConsoleSessionData : public nre::ServiceSession {
public:
	ConsoleSessionData(ConsoleService *srv,size_t id,capsel_t caps,nre::Pt::portal_func func)
			: ServiceSession(srv,id,caps,func), _has_screen(false), _page(0),  _sm(),
			  _in_ds(), _out_ds(), _prod(), _regs(), _pages(0), _srv(srv) {
		_regs.offset = nre::Console::TEXT_OFF >> 1;
		_regs.mode = 0;
		_regs.cursor_pos = (nre::Console::ROWS - 1) * nre::Console::COLS + (nre::Console::TEXT_OFF >> 1);
		_regs.cursor_style = 0x0d0e;
	}
	virtual ~ConsoleSessionData() {
		delete _prod;
		delete _in_ds;
		delete _out_ds;
	}

	virtual void invalidate() {
		if(_srv->active() == this) {
			// ensure that we don't have the screen; the session might be destroyed before the
			// viewswitcher can handle the switch and therefore, take away the screen, if necessary.
			to_back();
			// choose a different session
			_srv->next();
		}
	}

	size_t offset() const {
		return _regs.offset << 1;
	}
	uint page() const {
		return _page;
	}
	nre::Producer<nre::Console::ReceivePacket> *prod() {
		return _prod;
	}
	nre::DataSpace *out_ds() {
		return _out_ds;
	}

	void prev() {
		if(_pages != 0) {
			_page = (_page - 1) % _pages;
			_srv->switcher().switch_to(this,this);
		}
	}
	void next() {
		if(_pages != 0) {
			_page = (_page + 1) % _pages;
			_srv->switcher().switch_to(this,this);
		}
	}

	void create(nre::DataSpace *in_ds,nre::DataSpace *out_ds,uint pages);

	void to_front() {
		if(!_has_screen) {
			swap();
			activate();
			_has_screen = true;
		}
	}
	void to_back() {
		if(_has_screen) {
			swap();
			_has_screen = false;
		}
	}
	void activate() {
		if(_pages != 0)
			_regs.offset = (nre::Console::TEXT_OFF >> 1) + (_page << 11);
		set_regs(_regs);
	}

	void set_regs(const nre::Console::Register &regs) {
		if(_pages != 0)
			_page = (_regs.offset - (nre::Console::TEXT_OFF >> 1)) >> 11;
		_regs = regs;
		if(_srv->active() == this)
			_srv->screen()->set_regs(_regs);
	}

	PORTAL static void portal(capsel_t pid);

private:
	void swap() {
		_out_ds->switch_to(_srv->screen()->mem());
	}

	bool _has_screen;
	uint _page;
	nre::UserSm _sm;
	nre::DataSpace *_in_ds;
	nre::DataSpace *_out_ds;
	nre::Producer<nre::Console::ReceivePacket> *_prod;
	nre::Console::Register _regs;
	uint _pages;
	ConsoleService *_srv;
};
