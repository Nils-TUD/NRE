/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <dev/Console.h>
#include <util/DList.h>

#include "ConsoleService.h"

class ConsoleSessionData : public nul::SessionData {
public:
	ConsoleSessionData(nul::Service *s,size_t id,capsel_t caps,nul::Pt::portal_func func)
			: SessionData(s,id,caps,func), _page(0),  _sm(), _in_ds(), _out_ds(), _prod(), _regs(),
			  _show_pages(true) {
		_regs.offset = nul::Console::TEXT_OFF >> 1;
		_regs.mode = 0;
		_regs.cursor_pos = (nul::Console::ROWS - 1) * nul::Console::COLS + (nul::Console::TEXT_OFF >> 1);
		_regs.cursor_style = 0x0d0e;
	}
	virtual ~ConsoleSessionData() {
		delete _prod;
		delete _in_ds;
		delete _out_ds;
	}

	uint page() const {
		return _page;
	}
	nul::Producer<nul::Console::ReceivePacket> *prod() {
		return _prod;
	}
	nul::DataSpace *out_ds() {
		return _out_ds;
	}

	void prev() {
		if(_show_pages) {
			_page = (_page - 1) % Screen::TEXT_PAGES;
			ConsoleService::get()->switcher().switch_to(this,this);
		}
	}
	void next() {
		if(_show_pages) {
			_page = (_page + 1) % Screen::TEXT_PAGES;
			ConsoleService::get()->switcher().switch_to(this,this);
		}
	}

	void create(nul::DataSpace *in_ds,nul::DataSpace *out_ds,bool show_pages = true);

	void to_front() {
		swap();
		activate();
	}
	void to_back() {
		swap();
	}
	void activate() {
		_regs.offset = (nul::Console::TEXT_OFF >> 1) + (_page << 11);
		set_regs(_regs);
	}

	void set_regs(const nul::Console::Register &regs) {
		_page = (_regs.offset - (nul::Console::TEXT_OFF >> 1)) >> 11;
		_regs = regs;
		if(ConsoleService::get()->active() == this)
			ConsoleService::get()->screen()->set_regs(_regs);
	}

	PORTAL static void portal(capsel_t pid);

private:
	void swap() {
		_out_ds->switch_to(ConsoleService::get()->screen()->mem());
	}

	uint _page;
	nul::UserSm _sm;
	nul::DataSpace *_in_ds;
	nul::DataSpace *_out_ds;
	nul::Producer<nul::Console::ReceivePacket> *_prod;
	nul::Console::Register _regs;
	bool _show_pages;
};
