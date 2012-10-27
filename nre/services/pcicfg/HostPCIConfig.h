/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009, Bernhard Kauer <bk@vmmon.org>
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

#include <kobj/Ports.h>
#include <kobj/UserSm.h>
#include <util/ScopedLock.h>

#include "Config.h"

class HostPCIConfig : public Config {
	static const uint PORT_ADDR = 0xCF8;
	static const uint PORT_DATA = 0xCFC;

public:
	explicit HostPCIConfig() : _sm(), _addr(PORT_ADDR, 4), _data(PORT_DATA, 4) {
	}

	virtual const char *name() const {
		return "PCIConfig";
	}
	virtual bool contains(bdf_type bdf, size_t offset) const {
		return offset < 0x100 && bdf < 0x10000;
	}
	virtual uintptr_t addr(bdf_type, size_t) {
		throw nre::Exception(nre::E_ARGS_INVALID, "ADDR cmd not supported in PCI config space");
	}
	virtual value_type read(bdf_type bdf, size_t offset) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		select(bdf, offset);
		return _data.in<uint32_t>();
	}
	virtual void write(bdf_type bdf, size_t offset, value_type value) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		select(bdf, offset);
		_data.out<uint32_t>(value);
	}
	void reset() {
		_addr.out<uint8_t>((_addr.in<uint8_t>(1) & ~4) | 0x02, 1);
		_addr.out<uint8_t>(0x06, 1);
		_addr.out<uint8_t>(0x01, 1);
	}

	bdf_type search_device(value_type theclass = ~0U, value_type subclass = ~0U, uint inst = ~0U);
	bdf_type search_bridge(value_type dst);

private:
	void select(bdf_type bdf, size_t offset) {
		uint32_t addr = 0x80000000 | (bdf << 8) | (offset & 0xFC);
		_addr.out<uint32_t>(addr);
	}

	nre::UserSm _sm;
	nre::Ports _addr;
	nre::Ports _data;
};
