/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2011, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Bernhard Kauer <bk@vmmon.org>
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

#include <kobj/Sc.h>
#include <stream/Serial.h>
#include <util/Date.h>
#include <util/Topology.h>
#include <Logging.h>

#include "HostTimer.h"
#include "HostHPET.h"
#include "HostPIT.h"

using namespace nre;

void HostTimer::ClientData::init(size_t _sid, cpu_t cpuno, HostTimer::PerCpu *per_cpu) {
	sm = new nre::Sm(0);
	nr = per_cpu->abstimeouts.alloc(this);
	cpu = cpuno;
	sid = _sid;
}

HostTimer::HostTimer(bool force_pit, bool force_hpet_legacy, bool slow_rtc)
	: _clocks_per_tick(0), _timer(), _rtc(), _clock(Timer::WALLCLOCK_FREQ), _per_cpu(), _xcpu_up(0) {
	if(!force_pit) {
		try {
			_timer = new HostHPET(force_hpet_legacy);
		}
		catch(const Exception &e) {
			Serial::get() << "TIMER: HPET initialization failed: " << e.msg() << "\n";
		}
	}
	if(force_pit || !_timer)
		_timer = new HostPIT(1000);

	// HPET: Counter is running, IRQs are off.
	// PIT:  PIT is programmed to run in periodic mode, if HPET didn't work for us.

	_clocks_per_tick = (static_cast<timevalue_t>(Hip::get().freq_tsc) * 1000 * CPT_RES) / _timer->freq();
	LOG(Logging::TIMER,
	    Serial::get().writef("TIMER: %Lu+%04Lu/%u TSC ticks per timer tick.\n",
	                         _clocks_per_tick / CPT_RES, _clocks_per_tick % CPT_RES, CPT_RES));

	// Get wallclock time
	if(slow_rtc)
		_rtc.sync();
	timevalue_t msecs = _rtc.timestamp();
	DateInfo date;
	Date::gmtime(msecs / Timer::WALLCLOCK_FREQ, &date);
	LOG(Logging::TIMER,
	    Serial::get().writef("TIMER: timestamp: %Lu secs\n", msecs / Timer::WALLCLOCK_FREQ));
	LOG(Logging::TIMER,
	    Serial::get().writef("TIMER: date: %02d.%02d.%04d %d:%02d:%02d\n",
	                         date.mday, date.mon, date.year, date.hour, date.min, date.sec));

	_timer->start(Math::muldiv128(msecs, _timer->freq(), Timer::WALLCLOCK_FREQ));

	// Initialize per cpu data structure
	_per_cpu = new PerCpu *[CPU::count()];
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		_per_cpu[it->log_id()] = new PerCpu(this, it->log_id());

	// Create remote slot mapping
	size_t n = CPU::count();
	size_t parts = _timer->timer_count();
	cpu_t cpu_cpu[n];
	cpu_t part_cpu[n];
	Topology::divide(CPU::begin(), CPU::end(), n, parts, part_cpu, cpu_cpu);

	// Bootstrap IRQ handlers. IRQs are disabled. Each worker enables its IRQ when it comes up.
	for(size_t i = 0; i < parts; i++) {
		cpu_t cpu = part_cpu[i];
		LOG(Logging::TIMER_DETAIL, Serial::get().writef("TIMER: CPU%u owns Timer%zu.\n", cpu, i));

		_per_cpu[cpu]->has_timer = true;
		_per_cpu[cpu]->timer = _timer->timer(i);
		_per_cpu[cpu]->timer->init(*_timer, cpu);

		// We allocate a couple of unused slots if there is an odd
		// combination of CPU count and usable timers. Who cares.
		_per_cpu[cpu]->slots = new RemoteSlot[n / parts];
	}

	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		cpu_t cpu = it->log_id();
		if(!_per_cpu[cpu]->has_timer) {
			PerCpu &remote = *_per_cpu[cpu_cpu[cpu]];

			_per_cpu[cpu]->remote_sm = &remote.xcpu_sm;
			RemoteSlot *rslot = &remote.slots[remote.slot_count];
			_per_cpu[cpu]->remote_slot = rslot;

			// Fake a ClientData for this CPU.
			rslot->data.sm = &_per_cpu[cpu]->xcpu_sm;
			rslot->data.abstimeout = 0;
			rslot->data.nr = remote.abstimeouts.alloc(&rslot->data);

			LOG(Logging::TIMER_DETAIL,
			    Serial::get().writef("TIMER: CPU%u maps to CPU%u slot %zu.\n",
			                         cpu, cpu_cpu[cpu], remote.slot_count));
			remote.slot_count++;
		}
	}

	_timer->update_ticks(true);
	uint xcpu_threads_started = 0;
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		cpu_t cpu = it->log_id();
		_timer->enable(_per_cpu[cpu]->timer, _per_cpu[cpu]->has_timer);

		// Enable XCPU threads for CPUs that either have to serve or
		// need to query other CPUs.
		if((_per_cpu[cpu]->slot_count > 0) || !_per_cpu[cpu]->has_timer) {
			// Create wakeup thread
			GlobalThread *gt = GlobalThread::create(xcpu_wakeup_thread, cpu, String("timer-wakeup"));
			gt->set_tls(Thread::TLS_PARAM, this);
			gt->start();
			xcpu_threads_started++;
		}
		if(_per_cpu[cpu]->has_timer) {
			GlobalThread *gt = GlobalThread::create(gsi_thread, cpu, String("timer-gsi"));
			gt->set_tls(Thread::TLS_PARAM, this);
			gt->start();
		}
	}

	// XXX Do we need those when we have enough timers for all CPUs?
	LOG(Logging::TIMER_DETAIL, Serial::get().writef(
	        "TIMER: Waiting for %u XCPU threads to come up.\n", xcpu_threads_started));
	while(xcpu_threads_started-- > 0)
		_xcpu_up.down();
	LOG(Logging::TIMER_DETAIL, Serial::get().writef("TIMER: Initialized!\n"));
}

bool HostTimer::per_cpu_handle_xcpu(PerCpu *per_cpu) {
	bool reprogram = false;

	// Process cross-CPU timeouts. slot_count is zero for CPUs without
	// a timer.
	for(size_t i = 0; i < per_cpu->slot_count; i++) {
		RemoteSlot &cur = per_cpu->slots[i];
		timevalue_t to;

		do {
			to = cur.data.abstimeout;
			// No need to program a timeout
			if(to == 0ULL)
				goto next;
		}
		while(!Atomic::swap<timevalue_t, timevalue_t>(&cur.data.abstimeout, to, 0));

		if(to < per_cpu->last_to)
			reprogram = true;

		per_cpu->abstimeouts.cancel(cur.data.nr);
		per_cpu->abstimeouts.request(cur.data.nr, to);

next:
		;
	}
	return reprogram;
}

bool HostTimer::per_cpu_client_request(PerCpu *per_cpu, ClientData *data) {
	unsigned nr = data->nr;
	per_cpu->abstimeouts.cancel(nr);

	timevalue_t t = absolute_tsc_to_timer(data->abstimeout);
	// XXX Set abstimeout to zero here?
	// timer in the past?
	if(t == 0) {
		LOG(Logging::TIMER_DETAIL, Serial::get().writef("Timeout in past\n"));
		data->sm->up();
		return false;
	}
	per_cpu->abstimeouts.request(nr, t);
	return (t < per_cpu->last_to);
}

// Returns the next timeout.
timevalue_t HostTimer::handle_expired_timers(PerCpu *per_cpu, timevalue_t now) {
	ClientData *data;
	uint nr;
	while((nr = per_cpu->abstimeouts.trigger(now, &data))) {
		assert(data);
		per_cpu->abstimeouts.cancel(nr);
		Atomic::add(&data->count, 1U);
		data->sm->up();
	}
	return per_cpu->abstimeouts.timeout();
}

void HostTimer::portal_per_cpu(capsel_t) {
	HostTimer *ht = Thread::current()->get_tls<HostTimer*>(Thread::TLS_PARAM);
	cpu_t cpu = CPU::current().log_id();
	PerCpu *per_cpu = ht->_per_cpu[cpu];

	UtcbFrameRef uf;
	WorkerMessage m;
	uf >> m;
	bool reprogram = false;

	// We jump here if we were to late with timer
	// programming. reprogram stays true.
again:
	switch(m.type) {
		case WorkerMessage::XCPU_REQUEST:
			reprogram = ht->per_cpu_handle_xcpu(per_cpu);
			break;
		case WorkerMessage::CLIENT_REQUEST:
			reprogram = ht->per_cpu_client_request(per_cpu, m.data);
			break;
		case WorkerMessage::TIMER_IRQ: {
			timevalue_t now = ht->_timer->update_ticks(false);
			ht->handle_expired_timers(per_cpu, now);
			reprogram = true;
			break;
		}
	}

	// Check if we don't need to reprogram or if we are in periodic
	// mode. Either way we are done.
	if(!reprogram || (ht->_timer->is_periodic() && per_cpu->has_timer))
		return;

	// Okay, we need to program a new timeout.
	timevalue_t next_to = per_cpu->abstimeouts.timeout();
	timevalue_t estimated_now = ht->_timer->last_ticks();
	// give the timer a chance to change the next timer we're about to program. this is used by
	// HPET to tick every once in a while to ensure proper overflow detection.
	next_to = ht->_timer->next_timeout(estimated_now, next_to);
	per_cpu->last_to = next_to;

	if(per_cpu->has_timer) {
		per_cpu->timer->program_timeout(next_to);
		// Check whether we might have missed that interrupt.
		if(ht->_timer->is_in_past(next_to)) {
			LOG(Logging::TIMER_DETAIL, Serial::get().writef("Missed interrupt...goto again\n"));
			m.type = WorkerMessage::TIMER_IRQ;
			goto again;
		}
	}
	else {
		// Tell timer_cpu that we have a new timeout.
		if(next_to == ~0ULL)
			return;

		// XXX Needs to be written atomically!
		per_cpu->remote_slot->data.abstimeout = next_to;
		Sync::memory_barrier();
		per_cpu->remote_sm->up();
	}
}

NORETURN void HostTimer::xcpu_wakeup_thread(void *) {
	HostTimer *ht = Thread::current()->get_tls<HostTimer*>(Thread::TLS_PARAM);
	cpu_t cpu = CPU::current().log_id();
	PerCpu *const our = ht->_per_cpu[cpu];
	ht->_xcpu_up.up();

	WorkerMessage m;
	m.type = our->has_timer ? WorkerMessage::XCPU_REQUEST : WorkerMessage::TIMER_IRQ;
	m.data = 0;
	while(1) {
		our->xcpu_sm.zero();

		UtcbFrame uf;
		uf << m;
		our->worker_pt.call(uf);
	}
}

NORETURN void HostTimer::gsi_thread(void *) {
	HostTimer *ht = Thread::current()->get_tls<HostTimer*>(Thread::TLS_PARAM);
	cpu_t cpu = CPU::current().log_id();
	PerCpu *const our = ht->_per_cpu[cpu];
	WorkerMessage m;
	m.type = WorkerMessage::TIMER_IRQ;
	m.data = 0;
	LOG(Logging::TIMER, Serial::get().writef("Listening to GSI %u\n", our->timer->gsi().gsi()));
	while(1) {
		our->timer->gsi().down();

		ht->_timer->ack_irq(our->timer);
		UtcbFrame uf;
		uf << m;
		our->worker_pt.call(uf);
	}
}
