/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
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
#include <services/TimeTracker.h>
#include <utcb/UtcbFrame.h>
#include <Syscalls.h>
#include <Logging.h>
#include "Admission.h"
#include "Hypervisor.h"

using namespace nre;

Connection *Admission::_tt_con;
TimeTrackerSession *Admission::_tt_sess;
UserSm Admission::_sm INIT_PRIO_ADM;
SList<Admission::SchedEntity> Admission::_list INIT_PRIO_ADM;

void Admission::notify_start(const String &name,cpu_t cpu,capsel_t sc) {
	try {
		if(!_tt_con) {
			ScopedPtr<Connection> con(new Connection("timetracker"));
			_tt_con = con.release();
			_tt_sess = new TimeTrackerSession(*_tt_con);

			// add idle Scs
			for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
				char name[32];
				OStringStream stream(name,sizeof(name));
				stream << "CPU" << it->log_id() << "-idle";
				capsel_t sc = Hypervisor::request_idle_sc(it->phys_id());
				_tt_sess->start(String(name),it->log_id(),sc,true);
			}
			// notify the timetracker about the already existing ones
			for(SList<Admission::SchedEntity>::iterator it = _list.begin(); it != _list.end(); ++it)
				_tt_sess->start(it->name(),it->cpu(),it->cap(),false);
		}

		_tt_sess->start(name,cpu,sc,false);
	}
	catch(...) {
		// ignore it
	}
}

void Admission::notify_stop(cpu_t cpu,capsel_t sc) {
	if(_tt_sess)
		_tt_sess->stop(cpu,sc);
}

void Admission::portal_sc(capsel_t) {
	UtcbFrameRef uf;
	try {
		Sc::Command cmd;
		uf >> cmd;

		switch(cmd) {
			case Sc::START: {
				String name;
				Qpd qpd;
				cpu_t cpu;
				capsel_t ec = uf.get_delegated(0).offset();
				uf >> name >> cpu >> qpd;
				uf.finish_input();

				ScopedCapSels sc;
				LOG(Logging::ADMISSION,
						Serial::get().writef("Root: Creating sc '%s' on cpu %u\n",name.str(),cpu));
				Syscalls::create_sc(sc.get(),ec,qpd,Pd::current()->sel());
				add_sc(new SchedEntity(name,cpu,sc.get()));

				uf.accept_delegates();
				uf.delegate(sc.release());
				uf << E_SUCCESS << qpd;
			}
			break;

			case Sc::STOP: {
				capsel_t sc = uf.get_translated(0).offset();
				uf.finish_input();

				SchedEntity *se = remove_sc(sc);
				LOG(Logging::ADMISSION,Serial::get().writef("Root: Destroying sc '%s' on cpu %u\n",
							se->name().str(),se->cpu()));
				CapRange(sc,1,Crd::OBJ_ALL).revoke(true);
				delete se;
				uf << E_SUCCESS;
			}
			break;
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}
