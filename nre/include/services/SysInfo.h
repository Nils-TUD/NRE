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

#include <arch/Types.h>
#include <ipc/Connection.h>
#include <ipc/PtClientSession.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

class SysInfoSession;

class SysInfo {
public:
	class TimeUser {
		friend class SysInfoSession;
	public:
		explicit TimeUser() : _name(), _cpu(), _time() {
		}

		const nre::String &name() const {
			return _name;
		}
		cpu_t cpu() const {
			return _cpu;
		}
		timevalue_t time() const {
			return _time;
		}

	private:
		nre::String _name;
		cpu_t _cpu;
		timevalue_t _time;
	};

	enum Command {
		GET_TOTALTIME,
		GET_TIMEUSER,
		GET_MEM,
	};
};

/**
 * Represents a session at the sysinfo service
 */
class SysInfoSession : public PtClientSession {
public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 */
	explicit SysInfoSession(Connection &con) : PtClientSession(con) {
	}

	void get_mem(size_t &total,size_t &free) {
		UtcbFrame uf;
		uf << SysInfo::GET_MEM;
		pt().call(uf);
		uf.check_reply();
		uf >> total >> free;
	}

	timevalue_t get_total_time(cpu_t cpu,bool update) {
		timevalue_t res;
		UtcbFrame uf;
		uf << SysInfo::GET_TOTALTIME << cpu << update;
		pt().call(uf);
		uf.check_reply();
		uf >> res;
		return res;
	}

	bool get_timeuser(size_t idx,SysInfo::TimeUser &tu) {
		UtcbFrame uf;
		uf << SysInfo::GET_TIMEUSER << idx;
		pt().call(uf);
		uf.check_reply();
		bool found;
		uf >> found;
		if(!found)
			return false;
		uf >> tu._name >> tu._cpu >> tu._time;
		return true;
	}
};

}
