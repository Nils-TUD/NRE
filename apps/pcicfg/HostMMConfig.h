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

	class MMConfigRange : public nul::SListItem {
	public:
		explicit MMConfigRange(uintptr_t base,uintptr_t start,size_t size)
			: _start(start), _size(size),
			  _ds(size * nul::ExecEnv::PAGE_SIZE,nul::DataSpaceDesc::LOCKED,nul::DataSpaceDesc::R,base),
			  _mmconfig(reinterpret_cast<uint*>(_ds.virt())) {
		}

		uintptr_t addr() const {
			return _ds.phys();
		}
		bool contains(uint32_t bdf,size_t offset) {
			return offset < 0x400 && nul::Math::in_range(bdf,_start,_size);
		}
		uint32_t read(uint32_t bdf,size_t offset) {
			return *field(bdf,offset);
		}
		void write(uint32_t bdf,size_t offset,uint32_t value) {
			*field(bdf,offset) = value;
		}

	private:
		uint *field(uint32_t bdf,size_t offset) const {
			return _mmconfig + (bdf << 10) + (offset & 0x3FF);
		}

	private:
		uintptr_t _start;
		size_t _size;
		nul::DataSpace _ds;
		uint *_mmconfig;
	};

public:
	explicit HostMMConfig();

	virtual bool contains(uint32_t bdf,size_t offset) const {
		for(nul::SList<MMConfigRange>::iterator it = _ranges.begin(); it != _ranges.end(); ++it) {
			if(it->contains(bdf,offset))
				return true;
		}
		return false;
	}
	virtual uintptr_t addr(uint32_t bdf,size_t offset) {
		MMConfigRange *range = find(bdf,offset);
		return range->addr();
	}
	virtual uint32_t read(uint32_t bdf,size_t offset) {
		nul::Serial::get() << "MMCONFIG: bdf=" << bdf << ", offset=" << offset << "\n";
		MMConfigRange *range = find(bdf,offset);
		return range->read(bdf,offset);
	}
	virtual void write(uint32_t bdf,size_t offset,uint32_t value) {
		MMConfigRange *range = find(bdf,offset);
		range->write(bdf,offset,value);
	}

private:
	MMConfigRange *find(uint32_t bdf,size_t offset) {
		for(nul::SList<MMConfigRange>::iterator it = _ranges.begin(); it != _ranges.end(); ++it) {
			if(it->contains(bdf,offset))
				return &*it;
		}
		throw nul::Exception(nul::E_NOT_FOUND);
	}

	nul::SList<MMConfigRange> _ranges;
};
