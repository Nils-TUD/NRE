/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <service/Connection.h>
#include <stream/OStringStream.h>
#include <dev/Timer.h>
#include <util/Clock.h>

#include "ViewSwitcher.h"
#include "ConsoleService.h"
#include "ConsoleSessionData.h"

using namespace nul;

char ViewSwitcher::_backup[Screen::COLS * 2];
char ViewSwitcher::_buffer[Screen::COLS + 1];

ViewSwitcher::ViewSwitcher() : _ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
		_prod(&_ds,true,true), _cons(&_ds,false), _ec(switch_thread,CPU::current().log_id()),
		_sc(&_ec,Qpd()) {
	_ec.set_tls<ViewSwitcher*>(Ec::TLS_PARAM,this);
}

void ViewSwitcher::switch_to(ConsoleSessionData *from,ConsoleSessionData *to) {
	SwitchCommand cmd;
	ConsoleSessionView *vfrom = from ? from->active() : 0;
	ConsoleSessionView *vto = to->active();
	cmd.oldviewid = vfrom ? vfrom->id() : -1;
	cmd.oldsessid = from ? from->id() : -1;
	cmd.sessid = to->id();
	cmd.viewid = vto ? vto->id() : -1;
	_prod.produce(cmd);
}

void ViewSwitcher::switch_to(ConsoleSessionView *from,ConsoleSessionView *to) {
	SwitchCommand cmd;
	cmd.oldviewid = from ? from->id() : -1;
	cmd.viewid = to->id();
	cmd.oldsessid = from ? from->session()->id() : -1;
	cmd.sessid = to->session()->id();
	_prod.produce(cmd);
}

void ViewSwitcher::switch_thread(void*) {
	ViewSwitcher *vs = Ec::current()->get_tls<ViewSwitcher*>(Ec::TLS_PARAM);
	ConsoleService *srv = ConsoleService::get();
	Clock clock(1000);
	Connection con("timer");
	TimerSession timer(con);
	timevalue_t until = 0;
	size_t sessid = 0;
	size_t viewid = 0;
	bool tag_done = false;
	while(1) {
		// are we finished?
		if(until && clock.source_time() >= until) {
			ScopedLock<RCULock> guard(&RCU::lock());
			ConsoleSessionData *sess = srv->get_session_by_id<ConsoleSessionData>(sessid);
			if(sess) {
				ConsoleSessionView *view = sess->get(viewid);
				// if we have a view, finally swap to that view. i.e. give him direct screen access
				if(view)
					view->swap();
				// if not, restore the first line
				else {
					char *screen = reinterpret_cast<char*>(srv->screen()->mem(sess->page()).virt());
					memcpy(screen,_backup,Screen::COLS * 2);
				}
			}
			until = 0;
		}

		// either block until the next request, or - if we're switching - check for new requests
		if(until == 0 || vs->_cons.has_data()) {
			SwitchCommand *cmd = vs->_cons.get();
			// if there is an old one, make a backup and detach him from screen
			if(cmd->oldsessid != (size_t)-1) {
				assert(cmd->oldsessid == sessid);
				ScopedLock<RCULock> guard(&RCU::lock());
				ConsoleSessionData *old = srv->get_session_by_id<ConsoleSessionData>(cmd->oldsessid);
				if(old) {
					ConsoleSessionView *view = old->get(cmd->oldviewid);
					// if there is a view and we have already given im the screen, take it away
					if(until == 0 && view)
						view->swap();
					// if there is no screen, restore the first line
					else if(!view) {
						char *screen = reinterpret_cast<char*>(srv->screen()->mem(old->page()).virt());
						memcpy(screen,_backup,Screen::COLS * 2);
					}
				}
			}
			sessid = cmd->sessid;
			viewid = cmd->viewid;
			// show the tag for 1sec
			until = clock.source_time(1000);
			tag_done = false;
			vs->_cons.next();
		}

		{
			ScopedLock<RCULock> guard(&RCU::lock());
			ConsoleSessionData *sess = srv->get_session_by_id<ConsoleSessionData>(sessid);
			// if the session is dead, stop switching to it
			if(!sess) {
				until = 0;
				continue;
			}

			// write console data
			ConsoleSessionView *view = sess->get(viewid);
			sess->set_page();
			if(view) {
				memcpy(reinterpret_cast<void*>(srv->screen()->mem(sess->page()).virt() + Screen::COLS * 2),
						reinterpret_cast<void*>(view->out_ds().virt() + Screen::COLS * 2),
						srv->screen()->mem(sess->page()).size() - Screen::COLS * 2);
			}

			if(!tag_done) {
				// write tag into buffer
				memset(_buffer,0,sizeof(_buffer));
				OStringStream os(_buffer,sizeof(_buffer));
				os << "Console " << sessid << "," << (viewid == (size_t)-1 ? 0 : viewid);

				char *screen = reinterpret_cast<char*>(srv->screen()->mem(sess->page()).virt());
				// backup line of screen (this is not necessary if we have a view, since it will be
				// copied back completely by the swap operation anyway)
				if(!view)
					memcpy(_backup,screen,Screen::COLS * 2);

				// write console tag
				char *s = _buffer;
				for(uint x = 0; x < Screen::COLS; ++x, ++s) {
					screen[x * 2] = *s ? *s : ' ';
					screen[x * 2 + 1] = COLOR;
				}
				tag_done = true;
			}
		}

		// wait 25ms
		timer.wait_until(clock.source_time(25));
	}
}
