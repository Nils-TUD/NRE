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
	/**
	 * The information about a global thread
	 */
	class TimeUser {
		friend class SysInfoSession;
	public:
		explicit TimeUser() : _name(), _cpu(), _time(), _totaltime() {
		}

		/**
		 * @return the thread name
		 */
		const nre::String &name() const {
			return _name;
		}
		/**
		 * @return the CPU it runs on
		 */
		cpu_t cpu() const {
			return _cpu;
		}
		/**
		 * @return the time run since the last update (in microseconds)
		 */
		timevalue_t time() const {
			return _time;
		}
		/**
		 * @return the total time run so far (in microseconds)
		 */
		timevalue_t totaltime() const {
			return _totaltime;
		}

	private:
		nre::String _name;
		cpu_t _cpu;
		timevalue_t _time;
		timevalue_t _totaltime;
	};

	/**
	 * The information about a child Pd
	 */
	class Child {
		friend class SysInfoSession;
	public:
		explicit Child() : _cmdline(), _virt(), _phys(), _threads() {
		}

		/**
		 * @return the command line
		 */
		const nre::String &cmdline() const {
			return _cmdline;
		}
		/**
		 * @return the total amount of virtual memory in use (in bytes)
		 */
		size_t virt_mem() const {
			return _virt;
		}
		/**
		 * @return the total amount of physical memory requested by this child (in bytes)
		 */
		size_t phys_mem() const {
			return _phys;
		}
		/**
		 * @return the number of threads
		 */
		size_t threads() const {
			return _threads;
		}

	private:
		nre::String _cmdline;
		size_t _virt;
		size_t _phys;
		size_t _threads;
	};

	/**
	 * The available commands
	 */
	enum Command {
		GET_TOTALTIME,
		GET_TIMEUSER,
		GET_MEM,
		GET_CHILD,
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

	/**
	 * Asks for information about the memory in the system
	 *
	 * @param total will be set to the total amount of physical memory available (in bytes)
	 * @param free will be set to the free physical memory (in bytes)
	 */
	void get_mem(size_t &total,size_t &free) {
		UtcbFrame uf;
		uf << SysInfo::GET_MEM;
		pt().call(uf);
		uf.check_reply();
		uf >> total >> free;
	}

	/**
	 * Asks for the total time spent on the given CPU. If <update> is set, it updates that time.
	 *
	 * @param cpu the CPU
	 * @param update whether to update this information
	 * @return the total time (in microseconds)
	 */
	timevalue_t get_total_time(cpu_t cpu,bool update) {
		timevalue_t res;
		UtcbFrame uf;
		uf << SysInfo::GET_TOTALTIME << cpu << update;
		pt().call(uf);
		uf.check_reply();
		uf >> res;
		return res;
	}

	/**
	 * Gets the TimeUser number <idx>, that is the global thread with given index.
	 *
	 * @param idx the index
	 * @param tu will be filled
	 * @return true if <idx> exists
	 */
	bool get_timeuser(size_t idx,SysInfo::TimeUser &tu) {
		UtcbFrame uf;
		uf << SysInfo::GET_TIMEUSER << idx;
		pt().call(uf);
		uf.check_reply();
		bool found;
		uf >> found;
		if(!found)
			return false;
		uf >> tu._name >> tu._cpu >> tu._time >> tu._totaltime;
		return true;
	}

	/**
	 * Gets the Child number <idx>.
	 *
	 * @param idx the index
	 * @param c will be filled
	 * @return true if <idx> exists
	 */
	bool get_child(size_t idx,SysInfo::Child &c) {
		UtcbFrame uf;
		uf << SysInfo::GET_CHILD << idx;
		pt().call(uf);
		uf.check_reply();
		bool found;
		uf >> found;
		if(!found)
			return false;
		uf >> c._cmdline >> c._virt >> c._phys >> c._threads;
		return true;
	}
};

}
