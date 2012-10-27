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
#include <util/CPUSet.h>

namespace nre {

/**
 * This class is used to configure a child. You can specify the CPUs presented to him, the
 * main-function and the modules. Later this will also be used to restrict access to resources.
 * You may create a subclass to have more flexibility regarding the modules.
 */
class ChildConfig {
public:
	static const size_t MAX_WAITS       = 4;

	enum ModuleAccess {
		OWN,                // access only to its own module
		FOLLOWING,          // access to its own and all following modules
		ALL,                // access to all modules
	};

	/**
	 * Creates a new child config for the module <no> in Hip
	 *
	 * @param no the number of the module in Hip
	 * @param cmdline the cmdline
	 * @param cpu the CPU for the main thread
	 */
	explicit ChildConfig(size_t no, const String &cmdline, cpu_t cpu = CPU::current().log_id())
		: _no(no), _last(false), _modaccess(OWN), _cpu(cpu), _cpus(), _entry(0), _waitcount(),
		  _waits(), _cmdline() {
		parse(cmdline);
	}
	virtual ~ChildConfig() {
	}

	/**
	 * @return the CPU for the main thread
	 */
	cpu_t cpu() const {
		return _cpu;
	}
	/**
	 * @return whether this should be the last module to load
	 */
	bool last() const {
		return _last;
	}

	/**
	 * @return the number of waits
	 */
	size_t waits() const {
		return _waitcount;
	}
	/**
	 * @param i the wait index
	 * @return the service name to wait for
	 */
	const String &wait(size_t i) const {
		return _waits[i];
	}

	/**
	 * @return the commandline
	 */
	const String &cmdline() const {
		return _cmdline;
	}

	/**
	 * @return the CPUs that should be marked available in the Hip
	 */
	const CPUSet &cpus() const {
		return _cpus;
	}
	void cpus(const CPUSet &cpus) {
		_cpus = cpus;
	}

	/**
	 * The entry of the module, i.e. the main-function to call
	 */
	uintptr_t entry() const {
		return _entry;
	}
	void entry(uintptr_t entry) {
		_entry = entry;
	}

	/**
	 * Stores the module <i> that should be provided to the child into <mem>, if available.
	 *
	 * @param i the index of the module
	 * @param mem the destination
	 * @return false if there should be no module <i>
	 */
	virtual bool get_module(size_t i, HipMem &mem) const {
		if(_modaccess == OWN && i > 0)
			return false;
		Hip::mem_iterator it = Hip::get().mem_begin();
		if(_modaccess == OWN || _modaccess == FOLLOWING)
			it += _no;
		for(; it != Hip::get().mem_end(); ++it) {
			if(it->type == HipMem::MB_MODULE) {
				if(i-- == 0)
					break;
			}
		}
		if(it == Hip::get().mem_end())
			return false;
		mem = *it;
		return true;
	}

private:
	void parse(const String &cmdline) {
		static char buffer[256];
		const char *str = cmdline.str();
		const char *start = str;
		size_t pos = 0;
		for(size_t i = 0; pos < sizeof(buffer) && i <= cmdline.length(); ++i) {
			if(i == cmdline.length() || str[i] == ' ') {
				size_t len = str + i - start;
				if(strncmp(start, "mods=following", len) == 0)
					_modaccess = FOLLOWING;
				else if(strncmp(start, "mods=all", len) == 0)
					_modaccess = ALL;
				else if(strncmp(start, "lastmod", 7) == 0)
					_last = true;
				else if(strncmp(start, "provides=", 9) == 0 && _waitcount < MAX_WAITS)
					_waits[_waitcount++] = String(start + 9, len - 9);
				else {
					if(pos + len + 1 >= sizeof(buffer))
						len = sizeof(buffer) - (pos + 2);
					memcpy(buffer + pos, start, len + 1);
					pos += len + 1;
				}
				start = str + i + 1;
			}
		}
		buffer[pos - 1] = '\0';
		_cmdline.reset(buffer, pos - 1);
	}

	size_t _no;
	bool _last;
	ModuleAccess _modaccess;
	cpu_t _cpu;
	CPUSet _cpus;
	uintptr_t _entry;
	size_t _waitcount;
	String _waits[MAX_WAITS];
	String _cmdline;
};

}
