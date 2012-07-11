/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Sm.h>
#include <CPU.h>

#include "bus/profile.h"
#include "Timeouts.h"

using namespace nre;

void Timeouts::timer_thread(void*) {
	Timeouts *to = Thread::current()->get_tls<Timeouts*>(Thread::TLS_PARAM);
	Sm &sm = to->_timer.sm(CPU::current().log_id());
	while(1) {
		sm.down();
		COUNTER_INC("timer");
		to->trigger();
	}
}

void Timeouts::trigger() {
	ScopedLock<UserSm> guard(&_sm);
	timevalue_t now = _mb.clock().source_time();
	// Force time reprogramming. Otherwise, we might not reprogram a
	// timer, if the timeout event reached us too early.
	_last_to = NO_TIMEOUT;

	// trigger all timeouts that are due
	size_t nr;
	while((nr = _timeouts.trigger(now))) {
		MessageTimeout msg(nr,_timeouts.timeout());
		_timeouts.cancel(nr);
		_mb.bus_timeout.send(msg);
	}
	program();
}

void Timeouts::program() {
	if(_timeouts.timeout() != NO_TIMEOUT) {
		timevalue next_to = _timeouts.timeout();
		if(next_to != _last_to) {
			_last_to = next_to;
			_timer.program(next_to);
		}
	}
}
