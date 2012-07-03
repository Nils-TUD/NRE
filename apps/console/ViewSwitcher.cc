/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <service/Connection.h>
#include <dev/Timer.h>
#include <util/Clock.h>

#include "ViewSwitcher.h"
#include "ConsoleService.h"
#include "ConsoleSessionData.h"

using namespace nul;

ViewSwitcher::ViewSwitcher() : _ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
		_prod(&_ds,true,true), _cons(&_ds,false), _ec(switch_thread,CPU::current().log_id()),
		_sc(&_ec,Qpd()) {
	_ec.set_tls<ViewSwitcher*>(Ec::TLS_PARAM,this);
	_sc.start();
}

void ViewSwitcher::switch_to(ConsoleSessionData *sess) {
	SwitchCommand cmd;
	cmd.sessid = sess->id();
	_prod.produce(cmd);
}

void ViewSwitcher::switch_thread(void*) {
	ViewSwitcher *vs = Ec::current()->get_tls<ViewSwitcher*>(Ec::TLS_PARAM);
	Clock clock(1000);
	Connection con("timer");
	TimerSession timer(con);
	timevalue_t until = 0;
	size_t sessid = 0;
	while(1) {
		Serial::get() << "sessid=" << sessid << ", until=" << until << ", now=" << clock.source_time() << "\n";
		// are we finished?
		if(until && clock.source_time() >= until) {
			ScopedLock<RCULock> guard(&RCU::lock());
			ConsoleSessionData *sess = ConsoleService::get()->get_session_by_id<ConsoleSessionData>(sessid);
			if(sess && sess->active())
				sess->active()->swap();
			until = 0;
		}

		// either block until the next request, or - if we're switching - check for new requests
		if(until == 0 || vs->_cons.has_data()) {
			SwitchCommand *cmd = vs->_cons.get();
			sessid = cmd->sessid;
			until = clock.source_time(1000);
			vs->_cons.next();
		}

		{
			ScopedLock<RCULock> guard(&RCU::lock());
			ConsoleService *srv = ConsoleService::get();
			ConsoleSessionData *sess = srv->get_session_by_id<ConsoleSessionData>(sessid);
			// if the session is dead, stop switching to it
			if(!sess) {
				until = 0;
				continue;
			}

			static char head[] = {'A',0x40,'B',0x40,'C',0x40,'D',0x40};
			ConsoleSessionView *view = sess->active();
			sess->set_page();
			if(view) {
				memcpy(reinterpret_cast<void*>(srv->screen()->mem().virt()),
						reinterpret_cast<void*>(view->out_ds().virt()),srv->screen()->mem().size());
			}
			memcpy(reinterpret_cast<void*>(srv->screen()->mem().virt()),head,sizeof(head));
		}

		// wait 1ms
		timer.wait_until(clock.source_time(1));
	}
}
