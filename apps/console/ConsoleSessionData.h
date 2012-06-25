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
#include <DList.h>

#include "ConsoleService.h"
#include "ConsoleSessionView.h"

class ConsoleSessionView;

class ConsoleSessionData : public nul::SessionData {
public:
	typedef nul::DList<ConsoleSessionView>::iterator iterator;

	ConsoleSessionData(nul::Service *s,size_t id,capsel_t caps,nul::Pt::portal_func func);
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
		iterator it = _view_cycler.current();
		if(it != _views.end())
			it->repaint();
	}

	uint create_view(nul::DataSpace *in_ds,nul::DataSpace *out_ds);
	bool destroy_view(uint view);

	PORTAL static void portal(capsel_t pid);

protected:
	virtual void accept_ds(nul::DataSpace *) {
		throw nul::ServiceException(nul::E_ARGS_INVALID);
	}

private:
	nul::UserSm _sm;
	nul::DList<ConsoleSessionView> _views;
	nul::Cycler<iterator> _view_cycler;
	nul::BitField<nul::Console::VIEW_COUNT> _view_ids;
};
