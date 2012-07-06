/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/LocalEc.h>
#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <dev/Timer.h>
#include <util/TimeoutList.h>

#include "HostTimerDevice.h"
#include "HostRTC.h"

class HostTimer {
	struct PerCpu;

public:
	enum {
		MAX_CLIENTS		= 64,
		// Resolution of our TSC clocks per HPET clock measurement. Lower
		// resolution mean larger error in HPET counter estimation.
		CPT_RES			= /* 1 divided by */ (1U<<13) /* clocks per hpet tick */
	};

	struct ClientData {
		// This field has different semantics: When this ClientData
		// belongs to a client it contains an absolute TSC value. If it
		// belongs to a remote CPU it contains an absolute timer count.
		volatile timevalue_t abstimeout;

		// How often has the timeout triggered?
		volatile uint count;

		uint nr;
		cpu_t cpu;
		nul::Sm *sm;

		explicit ClientData() : abstimeout(0), count(0), nr(0), cpu(0), sm(0) {
		}

		void init(cpu_t cpu,HostTimer::PerCpu *per_cpu);
	};

private:
	struct WorkerMessage {
		enum WMType {
			XCPU_REQUEST = 1,
			CLIENT_REQUEST,
			TIMER_IRQ,
		} type;
		ClientData *data;
	};

	struct RemoteSlot {
		ClientData data;
	};

	struct PerCpu {
		bool has_timer;
		HostTimerDevice::Timer *timer;
		nul::TimeoutList<MAX_CLIENTS,ClientData> abstimeouts;

		nul::LocalEc ec;
		nul::Pt worker_pt;
		nul::Sm xcpu_sm;
		timevalue_t last_to;

		// Used by CPUs without timer
		nul::Sm *remote_sm; // for cross cpu wakeup
		RemoteSlot *remote_slot; // where to store crosscpu timeouts

		// Used by CPUs with timer
		RemoteSlot *slots; // Array
		size_t slot_count; // with this many entries

		explicit PerCpu(HostTimer *ht,cpu_t cpu)
			: has_timer(false), timer(0), abstimeouts(), ec(cpu),
			  worker_pt(&ec,portal_per_cpu), xcpu_sm(0), last_to(~0ULL), remote_sm(),
			  remote_slot(), slots(), slot_count() {
			ec.set_tls(nul::Ec::TLS_PARAM,ht);
		}
	};

public:
	explicit HostTimer(bool force_pit = false,bool force_hpet_legacy = false,bool slow_rtc = false);

	void setup_clientdata(ClientData *data,cpu_t cpu) {
		data->init(cpu,_per_cpu[cpu]);
	}

	void program_timer(ClientData *data,timevalue_t time) {
        data->abstimeout = time;
        nul::UtcbFrame uf;
        WorkerMessage m;
        m.type = WorkerMessage::CLIENT_REQUEST;
        m.data = data;
        uf << m;
        _per_cpu[data->cpu]->worker_pt.call(uf);
	}

	void get_time(timevalue_t &uptime,timevalue_t &unixts) {
		timevalue_t ticks = _timer->current_ticks();
		uptime = _clock.dest_time();
		unixts = nul::Math::muldiv128(ticks,nul::Timer::WALLCLOCK_FREQ,_timer->freq());
	}

private:
	/**
	 * Convert an absolute TSC value into an absolute time counter value. Call only from
	 * per_cpu thread. Returns ZERO if result is in the past.
	 */
	timevalue_t absolute_tsc_to_timer(timevalue_t tsc) {
		int64_t diff = tsc - nul::Util::tsc();
		if(diff <= 0)
			return 0;
		diff = nul::Math::muldiv128(diff,CPT_RES,_clocks_per_tick);
		return diff + _timer->current_ticks();
	}

	bool per_cpu_handle_xcpu(PerCpu *per_cpu);
	bool per_cpu_client_request(PerCpu *per_cpu,ClientData *data);
	timevalue_t handle_expired_timers(PerCpu *per_cpu,timevalue_t now);

	PORTAL static void portal_per_cpu(capsel_t pid);
	NORETURN static void xcpu_wakeup_thread(void *);
	NORETURN static void gsi_thread(void *);

	timevalue_t _clocks_per_tick;
	HostTimerDevice *_timer;
	HostRTC _rtc;
	nul::Clock _clock;
	PerCpu **_per_cpu;
	nul::Sm _xcpu_up;
};
