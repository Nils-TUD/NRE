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

#pragma once

#include <kobj/GlobalThread.h>
#include <kobj/VCpu.h>
#include <ipc/ClientSession.h>
#include <ipc/Connection.h>
#include <utcb/UtcbFrame.h>
#include <String.h>
#include <Desc.h>

namespace nre {

class Admission {
public:
	enum Command {
		START
	};
};

class AdmissionSession : public ClientSession {
	class Init {
	public:
		Init();
	};

public:
	/**
	 * Creates an admission-session for the given global thread. Please call start() to finally
	 * start it.
	 *
	 * @param gt the global thread to start
	 * @param name the name for the Sc
	 * @param qpd the quantum+prio-descriptor
	 */
	explicit AdmissionSession(GlobalThread *gt,const String &name,Qpd qpd = Qpd())
			: ClientSession(*_con), _ec(gt), _name(name), _qpd(qpd) {
	}
	/**
	 * Creates an admission-session for the given VCPU. Please call start() to finally start it.
	 *
	 * @param vcpu the VCPU to start
	 * @param name the name for the Sc
	 * @param qpd the quantum+prio-descriptor (might be changed)
	 */
	explicit AdmissionSession(VCpu *vcpu,const String &name,Qpd qpd = Qpd())
			: ClientSession(*_con), _ec(vcpu), _name(name), _qpd(qpd) {
	}

	/**
	 * Finally starts the thread
	 */
	void start() {
		UtcbFrame uf;
		uf.delegate(_ec->sel());
		uf << Admission::START << _name << _ec->cpu() << _qpd;
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
		uf >> _qpd;
	}

	/**
	 * @return the ec
	 */
	Ec *ec() const {
		return _ec;
	}
	/**
	 * @return the name of the Sc
	 */
	const String &name() const {
		return _name;
	}
	/**
	 * @return the quantum + priority choose by the admission service
	 */
	const Qpd &qpd() const {
		return _qpd;
	}

private:
	Ec *_ec;
	String _name;
	Qpd _qpd;
	static Connection *_con;
	static Init _init;
};

}
