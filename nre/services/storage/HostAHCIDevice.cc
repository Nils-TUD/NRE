/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2008, Bernhard Kauer <bk@vmmon.org>
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

#include <Logging.h>

#include "HostAHCIDevice.h"

using namespace nre;

void HostAHCIDevice::init() {
	if(_regs->cmd & 0xc009) {
		// stop processing by clearing ST
		_regs->cmd &= ~1;
		if(wait_timeout(&_regs->cmd, 1 << 15, 0))
			throw Exception(E_FAILURE, 32, "Device %u: Unable to clear ST", _id);

		// stop FIS receiving
		_regs->cmd &= ~0x10;
		// wait until no fis is received anymore
		if(wait_timeout(&_regs->cmd, 1 << 14, 0))
			throw Exception(E_FAILURE, 32, "Device %u: Unable to stop FIS receiving", _id);
	}

	// set CL and FIS pointer
	addr2phys(_clds, _cl, &_regs->clb);
	addr2phys(_fisds, _fis, &_regs->fb);

	// clear error register
	_regs->serr = ~0;
	// and irq status register
	_regs->is = ~0;

	// enable FIS processing
	_regs->cmd |= 0x10;
	if(wait_timeout(&_regs->cmd, 1 << 15, 0))
		throw Exception(E_FAILURE, 32, "Device %u: Unable to enable FIS processing", _id);

	// CLO clearing
	_regs->cmd |= 0x8;
	uint res = wait_timeout(&_regs->cmd, 0x8, 0);
	// only report an error if CLO is still set. it seems to be ok if res is non-zero
	// at least, this is what I got from the AHCI spec and /drivers/ata/libahci.c in linux
	if(res & 0x8)
		throw Exception(E_FAILURE, 64, "Device %u: CLO did not clear (%#x)", _id, res);
	_regs->cmd |= 0x1;

	// nothing in progress anymore
	_inprogress = 0;

	// enable irqs
	_regs->ie = 0xf98000f1;
	identify_drive(_bufferds);
	//set_features(0x3, 0x46);
	//set_features(0x2, 0);
	//return identify_drive(buffer);
}

void HostAHCIDevice::readwrite(Producer<Storage::Packet> *prod, Storage::tag_type tag,
                               const DataSpace &ds, sector_type sector, const dma_type &dma,
                               bool write) {
	ScopedLock<UserSm> guard(&_sm);
	size_t length = dma.bytecount();
	// invalid offset or size?
	if(length >> 13)
		throw Exception(E_ARGS_INVALID, 64, "Device %u: Invalid sector count (%zu)", _id, length >> 13);

	uint8_t command = has_lba48() ? 0x25 : 0xc8;
	if(write)
		command = has_lba48() ? 0x35 : 0xca;
	set_command(command, sector, !write, length >> 13);

	for(dma_type::iterator it = dma.begin(); it != dma.end(); ++it) {
		if(it->offset > ds.size() || it->offset + it->count > ds.size()) {
			throw Exception(E_ARGS_INVALID, 64, "Device %u: Invalid offset(%zu)/count(%zu)",
			                it->offset, it->count);
		}
		add_dma(ds, it->offset, it->count);
	}
	start_command(prod, tag);
}

void HostAHCIDevice::irq() {
	uint32_t is = _regs->is;

	// clear interrupt status
	_regs->is = is;

	for(uint done = _inprogress & ~_regs->ci, tag; done; done &= ~(1 << tag)) {
		tag = nre::Math::bit_scan_forward(done);
		LOG(nre::Logging::STORAGE_DETAIL,
		    nre::Serial::get().writef("Operation for user %lx is finished\n", _usertags[tag].tag));
		if(_usertags[tag].prod)
			_usertags[tag].prod->produce(nre::Storage::Packet(_usertags[tag].tag, 0));

		_usertags[tag].tag = ~0;
		_inprogress &= ~(1 << tag);
	}

	if((_regs->tfd & 1) && (~_regs->tfd & 0x400)) {
		LOG(nre::Logging::STORAGE, nre::Serial::get().writef("command failed with %x\n", _regs->tfd));
		init();
	}
}

void HostAHCIDevice::set_command(uint8_t command, uint64_t sector, bool read, uint count, bool atapi,
                                 uint pmp, uint features) {
	_cl[_tag * CL_DWORDS + 0] = (atapi ? 0x20 : 0) | (read ? 0 : 0x40) | 5 | ((pmp & 0xf) << 12);
	_cl[_tag * CL_DWORDS + 1] = 0;

	// link command list and tables
	addr2phys(_ctds, _ct + _tag * (128 + MAX_PRD_COUNT * 16) / 4, _cl + _tag * CL_DWORDS + 2);

	// XXX Does any one know how to avoid these type casts in C++0x mode?
#define UC(x) static_cast<uint8_t>(x)
	uint8_t cfis[20] = {0x27, UC(0x80 | (pmp & 0xf)), command, UC(features), UC(sector),
		                UC(sector >> 8), UC(sector >> 16), 0x40, UC(sector >> 24), UC(sector >> 32),
		                UC(sector >> 40), UC(features >> 8), UC(count), UC(count >> 8), 0, 0, 0, 0, 0, 0};
	memcpy(_ct + _tag * (128 + MAX_PRD_COUNT * 16) / 4, cfis, sizeof(cfis));
}

void HostAHCIDevice::add_dma(const nre::DataSpace &ds, size_t offset, uint bytes) {
	uint32_t prd = _cl[_tag * CL_DWORDS] >> 16;
	if(prd >= MAX_PRD_COUNT)
		throw nre::Exception(nre::E_ARGS_INVALID, 32, "Device %u: No free PRD slot", _id);
	_cl[_tag * CL_DWORDS] += 1 << 16;
	uint32_t *p = _ct + ((_tag * (128 + MAX_PRD_COUNT * 16) + 0x80 + prd * 16) >> 2);
	addr2phys(ds, reinterpret_cast<void*>(ds.virt() + offset), p);
	p[3] = bytes - 1;
}

void HostAHCIDevice::add_prd(const nre::DataSpace &ds, uint bytes) {
	uint32_t prd = _cl[_tag * CL_DWORDS] >> 16;
	assert(~bytes & 1);
	assert(!(bytes >> 22));
	if(prd >= MAX_PRD_COUNT)
		throw nre::Exception(nre::E_ARGS_INVALID, 32, "Device %u: No free PRD slot", _id);
	_cl[_tag * CL_DWORDS] += 1 << 16;
	uint32_t *p = _ct + ((_tag * (128 + MAX_PRD_COUNT * 16) + 0x80 + prd * 16) >> 2);
	addr2phys(ds, reinterpret_cast<void*>(ds.virt()), p);
	p[3] = bytes - 1;
}

size_t HostAHCIDevice::start_command(nre::Producer<nre::Storage::Packet> *prod, ulong usertag) {
	// remember work in progress commands
	assert(!(_inprogress & (1 << _tag)));
	_inprogress |= 1 << _tag;
	_usertags[_tag].tag = usertag;
	_usertags[_tag].prod = prod;

	_regs->ci = 1 << _tag;
	size_t res = _tag;
	_tag = (_tag + 1) % _max_slots;
	return res;
}

void HostAHCIDevice::identify_drive(nre::DataSpace &buffer) {
	uint16_t *buf = reinterpret_cast<uint16_t*>(buffer.virt());
	memset(reinterpret_cast<void*>(buffer.virt()), 0, 512);
	set_command(0xec, 0, true);
	add_prd(buffer, 512);
	size_t tag = start_command(0, 0);

	// there is no IRQ on identify, as this is PIO data-in command
	if(wait_timeout(&_regs->ci, 1 << tag, 0))
		throw Exception(E_TIMEOUT, 64, "Device %u: Timeout while waiting on IDENTIFY to finish", _id);
	_inprogress &= ~(1 << tag);

	// we do not support spinup
	// TODO is 0 in qemu!? assert(buf[2] == 0xc837);
	memcpy(&_info, buf, 512);
	set_parameters();
}

uint HostAHCIDevice::set_features(uint features, uint count) {
	set_command(0xef, 0, false, count, false, 0, features);
	size_t tag = start_command(0, 0);

	// there is no IRQ on set_features, as this is a PIO command
	check3(wait_timeout(&_regs->ci, 1 << tag, 0));
	_inprogress &= ~(1 << tag);
	return 0;
}
