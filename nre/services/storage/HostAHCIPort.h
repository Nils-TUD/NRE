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

#pragma once

#include <mem/DataSpace.h>
#include <util/Clock.h>
#include <Assert.h>

#include "HostATA.h"

#define check3(X) { unsigned __res = X; if (__res) return __res; }

/**
 * A single AHCI port with its command list and receive FIS buffer.
 *
 * State: testing
 * Supports: read-sectors, write-sectors, identify-drive
 * Missing: ATAPI detection
 */
class HostAHCIPort {
	static const size_t CL_DWORDS = 8;
	static const size_t MAX_PRD_COUNT = 64;
	// timeout in milliseconds
	static const uint FREQ = 1000;
	static const uint TIMEOUT = 200;

	enum InterfacePowerManagement {
		IPM_NOT_PRESENT			= 0x0,
		IPM_ACTIVE				= 0x1,
	};

	enum DeviceDetection {
		DET_NOT_PRESENT			= 0x0,
		DET_PRESENT				= 0x3,
	};

	struct UserTag {
		nre::Producer<nre::Storage::Packet> *prod;
		nre::Storage::tag_type tag;
	};

public:
	enum {
		SATA_SIG_ATA	= 0x00000101,	// SATA drive
		SATA_SIG_ATAPI	= 0xEB140101,	// SATAPI drive
		SATA_SIG_SEMB	= 0xC33C0101,	// Enclosure management bridge
		SATA_SIG_PM		= 0x96690101,	// Port multiplier
		SATA_SIG_NONE	= 0xFFFFFFFF,	// No device attached
	};

	/**
	 * The register set of an AHCI port.
	 */
	union Register {
		struct {
			volatile uint32_t clb;
			volatile uint32_t clbu;
			volatile uint32_t fb;
			volatile uint32_t fbu;
			volatile uint32_t is;
			volatile uint32_t ie;
			volatile uint32_t cmd;
			volatile uint32_t res0;
			volatile uint32_t tfd;
			volatile uint32_t sig;
			volatile uint32_t ssts;
			volatile uint32_t sctl;
			volatile uint32_t serr;
			volatile uint32_t sact;
			volatile uint32_t ci;
			volatile uint32_t sntf;
			volatile uint32_t fbs;
		};
		volatile uint32_t regs[32];
	};

	static uint32_t get_signature(HostAHCIPort::Register *port) {
		uint32_t ssts = port->ssts;
		uint8_t ipm = (ssts >> 8) & 0x0F;
		uint8_t det = ssts & 0x0F;

		if(det != DET_PRESENT) // Check drive status
			return HostAHCIPort::SATA_SIG_NONE;
		if(ipm != IPM_ACTIVE)
			return HostAHCIPort::SATA_SIG_NONE;
		return port->sig;
	}

	explicit HostAHCIPort(Register *regs,uint disknr,size_t max_slots,bool dmar)
			: _sm(), _regs(regs), _clock(FREQ), _disknr(disknr), _max_slots(max_slots), _dmar(dmar),
			  _bufferds(512,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _clds(max_slots * CL_DWORDS * 4,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _ctds(max_slots * (32 + MAX_PRD_COUNT * 4) * 4,
					  nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _fisds(1024 * 4,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _cl(reinterpret_cast<uint32_t*>(_clds.virt())),
			  _ct(reinterpret_cast<uint32_t*>(_ctds.virt())),
			  _fis(reinterpret_cast<uint32_t*>(_fisds.virt())),
			  _tag(0), _params(), _usertags(), _inprogress() {
	}

	void get_params(nre::Storage::Parameter *params) const {
		_params.get_disk_parameter(params);
	}

	/**
	 * Initialize the port.
	 */
	uint init() {
		if(_regs->cmd & 0xc009) {
			// stop processing by clearing ST
			_regs->cmd &= ~1;
			check3(wait_timeout(&_regs->cmd,1 << 15,0));

			// stop FIS receiving
			_regs->cmd &= ~0x10;
			// wait until no fis is received anymore
			check3(wait_timeout(&_regs->cmd,1 << 14,0));
		}

		// set CL and FIS pointer
		addr2phys(_clds,_cl,&_regs->clb);
		addr2phys(_fisds,_fis,&_regs->fb);

		// clear error register
		_regs->serr = ~0;
		// and irq status register
		_regs->is = ~0;

		// enable FIS processing
		_regs->cmd |= 0x10;
		check3(wait_timeout(&_regs->cmd,1 << 15,0));

		// CLO clearing
		_regs->cmd |= 0x8;
		uint res = wait_timeout(&_regs->cmd,0x8,0);
		// only report an error if CLO is still set. it seems to be ok if res is non-zero
		// at least, this is what I got from the AHCI spec and /drivers/ata/libahci.c in linux
		if(res & 0x8)
			return res;
		_regs->cmd |= 0x1;

		// nothing in progress anymore
		_inprogress = 0;

		// enable irqs
		_regs->ie = 0xf98000f1;
		res = identify_drive(_bufferds);
		if(!res) {
			nre::Storage::Parameter info;
			_params.get_disk_parameter(&info);
			LOG(nre::Logging::STORAGE,
					nre::Serial::get().writef("%s drive #%zu present (%s)\n"
							"  %Lu sectors with of %zu bytes, max %zu requests\n",
							_regs->sig == SATA_SIG_ATA ? "SATA" : "SATAPI",
							_disknr,info.name,info.sectors,info.sector_size,info.max_requests));
		}
		return res;
		//set_features(0x3, 0x46);
		//set_features(0x2, 0);
		//return identify_drive(buffer);
	}

	void flush(nre::Producer<nre::Storage::Packet> *prod,nre::Storage::tag_type tag) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		set_command(_params._lba48 ? 0xea : 0xe7,0,true);
		start_command(prod,tag);
	}

	void readwrite(nre::Producer<nre::Storage::Packet> *prod,nre::Storage::tag_type tag,
			const nre::DataSpace &ds,nre::Storage::sector_type sector,size_t offset,
			size_t size,bool write) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		if(size & 0x1FF)
			throw nre::Exception(nre::E_ARGS_INVALID,32,"Can't transfer half sectors (%zu)",size);
		uint8_t command = _params._lba48 ? 0x25 : 0xc8;
		if(write)
			command = _params._lba48 ? 0x35 : 0xca;
		set_command(command,sector,!write,size >> 9);

		// invalid offset or size?
		if(offset + size < offset || offset + size > ds.size())
			throw nre::Exception(nre::E_ARGS_INVALID,64,"Invalid offset (%zu)/size (%zu)",offset,size);
		add_dma(ds,offset,size);
		start_command(prod,tag);
	}

	void debug() {
		nre::Serial::get().writef("AHCI is %x ci %x ie %x cmd %x tfd %x\n",
				_regs->is,_regs->ci,_regs->ie,_regs->cmd,_regs->tfd);
	}

	void irq() {
		uint32_t is = _regs->is;

		// clear interrupt status
		_regs->is = is;

		for(uint done = _inprogress & ~_regs->ci,tag; done; done &= ~(1 << tag)) {
			tag = nre::Math::bit_scan_forward(done);
			LOG(nre::Logging::STORAGE_DETAIL,
					nre::Serial::get().writef("Operation for user %lx is finished\n",_usertags[tag].tag));
			if(_usertags[tag].prod)
				_usertags[tag].prod->produce(nre::Storage::Packet(_usertags[tag].tag,0));

			_usertags[tag].tag = ~0;
			_inprogress &= ~(1 << tag);
		}

		if((_regs->tfd & 1) && (~_regs->tfd & 0x400)) {
			nre::Serial::get().writef("command failed with %x\n",_regs->tfd);
			init();
		}
	}

private:
	inline uint32_t wait_timeout(volatile uint32_t *reg,uint32_t mask,uint32_t value) {
		timevalue_t timeout = _clock.source_time(TIMEOUT);
		while(((*reg & mask) != value) && _clock.source_time() < timeout)
			nre::Util::pause();
		return (*reg & mask) != value;
	}

	/**
	 * Translate a virtual to a physical address.
	 */
	void addr2phys(const nre::DataSpace &ds,void *ptr,volatile uint32_t *dst) {
		uintptr_t value = reinterpret_cast<uintptr_t>(ptr);
		if(!_dmar)
			value = ds.phys() + (value & (nre::ExecEnv::PAGE_SIZE - 1));
		dst[0] = value;
		dst[1] = 0; // support 64bit mode
	}

	void set_command(uint8_t command,uint64_t sector,bool read,uint count = 0,bool atapi = false,
			uint pmp = 0,uint features = 0) {
		_cl[_tag * CL_DWORDS + 0] = (atapi ? 0x20 : 0) | (read ? 0 : 0x40) | 5 | ((pmp & 0xf) << 12);
		_cl[_tag * CL_DWORDS + 1] = 0;

		// link command list and tables
		addr2phys(_ctds,_ct + _tag * (128 + MAX_PRD_COUNT * 16) / 4,_cl + _tag * CL_DWORDS + 2);

		// XXX Does any one know how to avoid these type casts in C++0x mode?
#define UC(x) static_cast<uint8_t>(x)
		uint8_t cfis[20] = {0x27,UC(0x80 | (pmp & 0xf)),command,UC(features),UC(sector),
				UC(sector >> 8),UC(sector >> 16),0x40,UC(sector >> 24),UC(sector >> 32),
				UC(sector >> 40),UC(features >> 8),UC(count),UC(count >> 8),0,0,0,0,0,0};
		memcpy(_ct + _tag * (128 + MAX_PRD_COUNT * 16) / 4,cfis,sizeof(cfis));
	}

	bool add_dma(const nre::DataSpace &ds,size_t offset,uint count) {
		if((count & 1) || count >> 22)
			return true;

		uint32_t prd = _cl[_tag * CL_DWORDS] >> 16;
		if(prd >= MAX_PRD_COUNT)
			return true;
		_cl[_tag * CL_DWORDS] += 1 << 16;
		uint32_t *p = _ct + ((_tag * (128 + MAX_PRD_COUNT * 16) + 0x80 + prd * 16) >> 2);
		addr2phys(ds,reinterpret_cast<void*>(ds.virt() + offset),p);
		p[3] = count - 1;
		return false;
	}

	bool add_prd(const nre::DataSpace &ds,uint count) {
		uint32_t prd = _cl[_tag * CL_DWORDS] >> 16;
		assert(~count & 1);
		assert(!(count >> 22));
		if(prd >= MAX_PRD_COUNT)
			return true;
		_cl[_tag * CL_DWORDS] += 1 << 16;
		uint32_t *p = _ct + ((_tag * (128 + MAX_PRD_COUNT * 16) + 0x80 + prd * 16) >> 2);
		addr2phys(ds,reinterpret_cast<void*>(ds.virt()),p);
		p[3] = count - 1;
		return false;
	}

	size_t start_command(nre::Producer<nre::Storage::Packet> *prod,ulong usertag) {
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

	uint identify_drive(nre::DataSpace &buffer) {
		uint16_t *buf = reinterpret_cast<uint16_t*>(buffer.virt());
		memset(reinterpret_cast<void*>(buffer.virt()),0,512);
		set_command(0xec,0,true);
		add_prd(buffer,512);
		size_t tag = start_command(0,0);

		// there is no IRQ on identify, as this is PIO data-in command
		check3(wait_timeout(&_regs->ci,1 << tag,0));
		_inprogress &= ~(1 << tag);

		// we do not support spinup
		// TODO is 0 in qemu!? assert(buf[2] == 0xc837);
		return _params.update_params(buf,false);
	}

	uint set_features(uint features,uint count = 0) {
		set_command(0xef,0,false,count,false,0,features);
		size_t tag = start_command(0,0);

		// there is no IRQ on set_features, as this is a PIO command
		check3(wait_timeout(&_regs->ci,1 << tag,0));
		_inprogress &= ~(1 << tag);
		return 0;
	}

	nre::UserSm _sm;
	Register volatile *_regs;
	nre::Clock _clock;
	uint _disknr;
	size_t _max_slots;
	bool _dmar;
	nre::DataSpace _bufferds;
	nre::DataSpace _clds;
	nre::DataSpace _ctds;
	nre::DataSpace _fisds;
	uint32_t *_cl;
	uint32_t *_ct;
	uint32_t *_fis;
	size_t _tag;
	HostATA _params;
	UserTag _usertags[32];
	uint _inprogress;
};
