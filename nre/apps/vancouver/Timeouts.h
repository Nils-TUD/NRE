/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2009, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <services/Timer.h>
#include <util/TimeoutList.h>
#include <util/ScopedLock.h>

#include "bus/motherboard.h"

extern nre::UserSm globalsm;

class Timeouts {
	enum {
		NO_TIMEOUT	= ~0ULL
	};

public:
	Timeouts(Motherboard &mb) : _mb(mb), _sm(), _timeouts(), _timercon("timer"),
			_timer(_timercon), _last_to(NO_TIMEOUT) {
		nre::GlobalThread *gt = nre::GlobalThread::create(
				timer_thread,nre::CPU::current().log_id(),nre::String("vmm-timeouts"));
		gt->set_tls<Timeouts*>(nre::Thread::TLS_PARAM,this);
		gt->start();
	}

	size_t alloc() {
		// TODO we need a semaphore here, otherwise the compiler seems to generate wrong code in
		// release mode on x86_64. at least, I have no other explanation so far.
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		return _timeouts.alloc();
	}

	void request(size_t nr,timevalue_t to) {
		// TODO it can't be correct to not grab a lock here, because we access e.g. program() from
		// different threads here. but we have to grab globalsm in timer_thread and if we grab it
		// here, we deadlock ourself. But in NUL it works the same way, so we keep it for now.
		_timeouts.request(nr,to);
		program();
	}

	void time(timevalue_t &uptime,timevalue_t &unixtime) {
		_timer.get_time(uptime,unixtime);
	}

private:
	NORETURN static void timer_thread(void*);
	void trigger();
	void program();

	Motherboard &_mb;
	nre::UserSm _sm;
	nre::TimeoutList<32,void> _timeouts;
	nre::Connection _timercon;
	nre::TimerSession _timer;
	timevalue_t _last_to;
};
