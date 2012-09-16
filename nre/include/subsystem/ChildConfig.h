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
	enum ModuleAccess {
		OWN,				// access only to its own module
		FOLLOWING			// access to its own and all following modules
	};

	/**
	 * Creates a new child config for the module <no> in Hip
	 *
	 * @param no the number of the module in Hip
	 */
	explicit ChildConfig(size_t no) : _no(no), _modaccess(OWN), _cpus(), _entry(0) {
	}
	virtual ~ChildConfig() {
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
	 * Sets the module access type
	 */
	void module_access(ModuleAccess access) {
		_modaccess = access;
	}
	/**
	 * @return true if it should get the module <i>
	 */
	virtual bool has_module_access(size_t i) const {
		return (_modaccess == OWN) ? (i == _no) : (i >= _no);
	}
	/**
	 * @return the command line of module <i>
	 */
	virtual const char *module_cmdline(size_t i) const {
		Hip::mem_iterator mem = Hip::get().mem_begin() + i;
		if(mem->type == HipMem::MB_MODULE)
			return mem->cmdline();
		return 0;
	}

private:
	size_t _no;
	ModuleAccess _modaccess;
	CPUSet _cpus;
	uintptr_t _entry;
};

}
