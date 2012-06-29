/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <RCU.h>
#include <cstring>

#include "ConsoleSessionData.h"

using namespace nul;

ConsoleSessionData::ConsoleSessionData(Service *s,uint page,size_t id,capsel_t caps,Pt::portal_func func)
	: SessionData(s,id,caps,func), _page(page), _next_id(), _sm(), _views(),
	  _view_cycler(_views.begin(),_views.end()) {
}

ConsoleSessionData::~ConsoleSessionData() {
	while(_views.length() > 0) {
		ConsoleSessionView *view = &*_views.begin();
		RCU::invalidate(view);
		_views.remove(view);
	}
}

ConsoleSessionView *ConsoleSessionData::create_view(nul::DataSpace *in_ds) {
	ScopedLock<UserSm> guard(&_sm);
	uint id = _next_id++;
	ScopedPtr<ConsoleSessionView> v(new ConsoleSessionView(this,id,in_ds));
	iterator it = _views.append(v.get());
	ConsoleService::get()->active()->to_back();
	_view_cycler.reset(_views.begin(),it,_views.end());
	to_front();
	return v.release();
}

bool ConsoleSessionData::destroy_view(uint view) {
	ScopedLock<UserSm> guard(&_sm);
	for(iterator it = _views.begin(); it != _views.end(); ++it) {
		if(it->id() == view) {
			_views.remove(&*it);
			if(is_active() && _view_cycler.current() != it)
				_view_cycler.current()->swap();
			_view_cycler.reset(_views.begin(),_views.begin(),_views.end());
			to_front();
			RCU::invalidate(&*it);
			return true;
		}
	}
	return false;
}

void ConsoleSessionData::portal(capsel_t pid) {
	UtcbFrameRef uf;
	try {
		ScopedLock<RCULock> guard(&RCU::lock());
		ConsoleSessionData *sess = ConsoleService::get()->get_session<ConsoleSessionData>(pid);
		Console::ViewCommand cmd;
		uf >> cmd;
		switch(cmd) {
			case Console::CREATE_VIEW: {
				DataSpaceDesc indesc;
				capsel_t insel = uf.get_delegated(0).offset();
				uf >> indesc;
				uf.finish_input();

				ConsoleSessionView *view = sess->create_view(new DataSpace(indesc,insel));

				uf.accept_delegates();
				uf.delegate(view->out_ds().sel());
				uf << E_SUCCESS << view->id() << view->out_ds().desc();
			}
			break;

			case Console::DESTROY_VIEW: {
				uint view;
				uf >> view;
				uf.finish_input();

				if(sess->destroy_view(view))
					uf << E_SUCCESS;
				else
					uf << E_NOT_FOUND;
			}
			break;
		}
	}
	catch(const Exception &e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}
