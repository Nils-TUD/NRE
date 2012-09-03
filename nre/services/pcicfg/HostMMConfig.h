/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
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

#pragma once

#include <arch/Types.h>
#include <mem/DataSpace.h>
#include <util/SList.h>
#include <util/Math.h>
#include <Compiler.h>

#include "Config.h"

class HostMMConfig : public Config {
	struct AcpiMCFG {
		uint32_t magic;
		uint32_t len;
		uint8_t rev;
		uint8_t checksum;
		uint8_t oem_id[6];
		uint8_t model_id[8];
		uint32_t oem_rev;
		uint32_t creator_vendor;
		uint32_t creator_utility;
		uint8_t _res[8];
		struct Entry {
			uint64_t base;
			uint16_t pci_seg;
			uint8_t pci_bus_start;
			uint8_t pci_bus_end;
			uint32_t _res;
		} PACKED entries[];
	} PACKED;

	class MMConfigRange : public nre::SListItem {
	public:
		explicit MMConfigRange(uintptr_t base,uintptr_t start,size_t size)
			: _start(start), _size(size),
			  _ds(size * nre::ExecEnv::PAGE_SIZE,nre::DataSpaceDesc::LOCKED,nre::DataSpaceDesc::R,base),
			  _mmconfig(reinterpret_cast<uint*>(_ds.virt())) {
		}

		uintptr_t addr() const {
			return _ds.phys();
		}
		bool contains(bdf_type bdf,size_t offset) {
			return offset < 0x400 && nre::Math::in_range(bdf,_start,_size);
		}
		value_type read(bdf_type bdf,size_t offset) {
			return *field(bdf,offset);
		}
		void write(bdf_type bdf,size_t offset,value_type value) {
			*field(bdf,offset) = value;
		}

	private:
		uint *field(bdf_type bdf,size_t offset) const {
			return _mmconfig + (bdf << 10) + (offset & 0x3FF);
		}

	private:
		uintptr_t _start;
		size_t _size;
		nre::DataSpace _ds;
		uint *_mmconfig;
	};

public:
	explicit HostMMConfig();

	virtual const char *name() const {
		return "MMConfig";
	}
	virtual bool contains(bdf_type bdf,size_t offset) const {
		for(nre::SList<MMConfigRange>::iterator it = _ranges.begin(); it != _ranges.end(); ++it) {
			if(it->contains(bdf,offset))
				return true;
		}
		return false;
	}
	virtual uintptr_t addr(bdf_type bdf,size_t offset) {
		MMConfigRange *range = find(bdf,offset);
		return range->addr();
	}
	virtual value_type read(bdf_type bdf,size_t offset) {
		MMConfigRange *range = find(bdf,offset);
		return range->read(bdf,offset);
	}
	virtual void write(bdf_type bdf,size_t offset,value_type value) {
		MMConfigRange *range = find(bdf,offset);
		range->write(bdf,offset,value);
	}

private:
	MMConfigRange *find(bdf_type bdf,size_t offset) {
		for(nre::SList<MMConfigRange>::iterator it = _ranges.begin(); it != _ranges.end(); ++it) {
			if(it->contains(bdf,offset))
				return &*it;
		}
		throw nre::Exception(nre::E_NOT_FOUND,64,"Unable to find bdf %#x+%#x in MMConfig",bdf,offset);
	}

	nre::SList<MMConfigRange> _ranges;
};
