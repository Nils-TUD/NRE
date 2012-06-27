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

	virtual void invalidate();

	ConsoleSessionView *active() {
		iterator it = _view_cycler.current();
		return it != _views.end() ? &*it : 0;
	}
	bool is_active() const {
		return ConsoleService::get()->is_active(this);
	}
	bool is_active(const ConsoleSessionView *view) const {
		return is_active() && view == &*_view_cycler.current();
	}

	iterator prev() {
		return _view_cycler.prev();
	}
	iterator next() {
		return _view_cycler.next();
	}

	void repaint() {
		if(_page == PAGE_USER) {
			iterator it = _view_cycler.current();
			if(it != _views.end()) {
				ConsoleService::get()->screen()->set_page(it->uid(),_page);
				it->repaint();
			}
		}
		else
			ConsoleService::get()->screen()->set_page(_page,_page);
	}

	uint create_view(nul::DataSpace *in_ds,nul::DataSpace *out_ds);
	bool destroy_view(uint view);

	PORTAL static void portal(capsel_t pid);

protected:
	virtual void accept_ds(nul::DataSpace *) {
		throw nul::ServiceException(nul::E_ARGS_INVALID);
	}

private:
	uint _page;
	uint _next_id;
	nul::UserSm _sm;
	nul::DList<ConsoleSessionView> _views;
	nul::Cycler<iterator> _view_cycler;
};
