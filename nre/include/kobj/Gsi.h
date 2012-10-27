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

#include <kobj/Sm.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nre {

/**
 * Represents a global system interrupt. Creation of a GSI allocates it from the parent and
 * destruction release it again. By doing a down() or zero() on this object you can wait for
 * the interrupt.
 */
class Gsi : public Sm {
public:
	enum Op {
		ALLOC,
		RELEASE
	};

	/**
	 * Allocates the given GSI from the parent
	 *
	 * @param gsi the GSI
	 * @param cpu the CPU to route the gsi to
	 */
	explicit Gsi(uint gsi, cpu_t cpu = CPU::current().log_id()) : Sm(alloc(gsi, 0, cpu), true) {
		// neither keep the cap nor the selector
		set_flags(0);
	}

	/**
	 * Allocates a new GSI for the device specified by the given PCI configuration space location
	 *
	 * @param pcicfg the location in the PCI configuration space
	 * @param cpu the CPU to route the gsi to
	 */
	explicit Gsi(void *pcicfg, cpu_t cpu = CPU::current().log_id()) : Sm(alloc(0, pcicfg, cpu), true) {
		// neither keep the cap nor the selector
		set_flags(0);
	}

	/**
	 * Releases the GSI
	 */
	virtual ~Gsi() {
		release();
	}

	/**
	 * @return the GSI
	 */
	uint gsi() const {
		return _gsi;
	}
	/**
	 * @return the MSI address to program into the device (only set if its a MSI)
	 */
	uint64_t msi_addr() const {
		return _msi_addr;
	}
	/**
	 * @return the MSI value to program into the device (only set if its a MSI)
	 */
	word_t msi_value() const {
		return _msi_value;
	}

private:
	capsel_t alloc(uint gsi, void *pcicfg, cpu_t cpu) {
		UtcbFrame uf;
		ScopedCapSels cap;
		uf.delegation_window(Crd(cap.get(), 0, Crd::OBJ_ALL));
		uf << ALLOC << gsi << pcicfg;
		CPU::current().gsi_pt().call(uf);

		uf.check_reply();
		uf >> _gsi;
		Syscalls::assign_gsi(cap.get(), CPU::get(cpu).phys_id(), pcicfg, &_msi_addr, &_msi_value);
		return cap.release();
	}
	void release() {
		try {
			UtcbFrame uf;
			uf << RELEASE << _gsi;
			CPU::current().gsi_pt().call(uf);
		}
		catch(...) {
			// ignore
		}
	}

	uint64_t _msi_addr;
	word_t _msi_value;
	uint _gsi;
};

}
