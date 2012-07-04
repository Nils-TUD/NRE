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
#include "ConsoleSessionView.h"

class ConsoleSessionView;

class ConsoleSessionData : public nul::SessionData {
public:
	enum {
		PAGE_BOOT,
		PAGE_HV,
		PAGE_USER
	};

	typedef nul::DList<ConsoleSessionView>::iterator iterator;

	ConsoleSessionData(nul::Service *s,uint page,size_t id,capsel_t caps,nul::Pt::portal_func func);
	virtual ~ConsoleSessionData();

	ConsoleSessionView *active() {
		iterator it = _view_cycler.current();
		return it != _views.end() ? &*it : 0;
	}
	ConsoleSessionView *get(uint id) {
		for(iterator it = _views.begin(); it != _views.end(); ++it) {
			if(it->id() == id)
				return &*it;
		}
		return 0;
	}

	void prev() {
		if(_views.length() > 1) {
			iterator cur = _view_cycler.current();
			ConsoleService::get()->switcher().switch_to(&*cur,&*_view_cycler.prev());
		}
	}
	void next() {
		if(_views.length() > 1) {
			iterator cur = _view_cycler.current();
			ConsoleService::get()->switcher().switch_to(&*cur,&*_view_cycler.next());
		}
	}

	uint page() const {
		return _page;
	}
	void set_page() {
		if(_page == PAGE_USER) {
			iterator it = _view_cycler.current();
			if(it != _views.end()) {
				ConsoleService::get()->screen()->set_page(it->uid(),_page);
				return;
			}
		}
		ConsoleService::get()->screen()->set_page(_page,_page);
	}

	ConsoleSessionView *create_view(nul::DataSpace *in_ds,nul::DataSpace *out_ds);
	bool destroy_view(uint view);

	PORTAL static void portal(capsel_t pid);

private:
	uint _page;
	uint _next_id;
	nul::UserSm _sm;
	nul::DList<ConsoleSessionView> _views;
	nul::Cycler<iterator> _view_cycler;
};
