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
#include <ipc/Producer.h>
#include <util/Clock.h>
#include <Assert.h>

#include "Device.h"

#define check3(X) { unsigned __res = X; if (__res) return __res; }

/**
 * A single AHCI port with its command list and receive FIS buffer.
 *
 * State: testing
 * Supports: read-sectors, write-sectors, identify-drive
 * Missing: ATAPI detection
 */
class HostAHCIDevice : public Device {
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
	enum Signature {
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
			volatile uint32_t clb;		// command list base address, 1K-byte aligned
			volatile uint32_t clbu;	// command list base address upper 32 bits
			volatile uint32_t fb;		// FIS base address, 256-byte aligned
			volatile uint32_t fbu;		// FIS base address upper 32 bits
			volatile uint32_t is;		// interrupt status
			volatile uint32_t ie;		// interrupt enable
			volatile uint32_t cmd;		// command and status
			volatile uint32_t : 32;	// reserved
			volatile uint32_t tfd;		// task file data
			volatile uint32_t sig;		// signature
			volatile uint32_t ssts;	// SATA status (SCR0:SStatus)
			volatile uint32_t sctl;	// SATA control (SCR2:SControl)
			volatile uint32_t serr;	// SATA error (SCR1:SError)
			volatile uint32_t sact;	// SATA active (SCR3:SActive)
			volatile uint32_t ci;		// command issue
			volatile uint32_t sntf;	// SATA notification (SCR4:SNotification)
			volatile uint32_t fbs;		// FIS-based switch control
		};
		volatile uint32_t regs[32];
	};

	static uint32_t get_signature(HostAHCIDevice::Register *port) {
		uint32_t ssts = port->ssts;
		uint8_t ipm = (ssts >> 8) & 0x0F;
		uint8_t det = ssts & 0x0F;

		if(det != DET_PRESENT) // Check drive status
			return HostAHCIDevice::SATA_SIG_NONE;
		if(ipm != IPM_ACTIVE)
			return HostAHCIDevice::SATA_SIG_NONE;
		return port->sig;
	}

	explicit HostAHCIDevice(Register *regs,uint disknr,size_t max_slots,bool dmar)
			: Device(disknr), _sm(), _regs(regs), _clock(FREQ), _max_slots(max_slots), _dmar(dmar),
			  _bufferds(512,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _clds(max_slots * CL_DWORDS * 4,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _ctds(max_slots * (32 + MAX_PRD_COUNT * 4) * 4,
					  nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _fisds(1024 * 4,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW),
			  _cl(reinterpret_cast<uint32_t*>(_clds.virt())),
			  _ct(reinterpret_cast<uint32_t*>(_ctds.virt())),
			  _fis(reinterpret_cast<uint32_t*>(_fisds.virt())),
			  _tag(0), _usertags(), _inprogress() {
		init();
	}

	virtual const char *type() const {
		return is_atapi() ? "SATAPI" : "SATA";
	}
	virtual void determine_capacity() {
		_capacity = has_lba48() ? _info.lba48MaxLBA : _info.userSectorCount;
	}

	void flush(nre::Producer<nre::Storage::Packet> *prod,nre::Storage::tag_type tag) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		set_command(has_lba48() ? 0xea : 0xe7,0,true);
		start_command(prod,tag);
	}
	void readwrite(nre::Producer<nre::Storage::Packet> *prod,nre::Storage::tag_type tag,
			const nre::DataSpace &ds,sector_type sector,const dma_type &dma,bool write);
	void irq();

	void debug() {
		nre::Serial::get().writef("AHCI is %x ci %x ie %x cmd %x tfd %x tag %zx\n",
				_regs->is,_regs->ci,_regs->ie,_regs->cmd,_regs->tfd,_tag);
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
			value = ds.phys() + (value - ds.virt());
		dst[0] = value;
		dst[1] = 0; // support 64bit mode
	}

	void init();
	void set_command(uint8_t command,uint64_t sector,bool read,uint count = 0,bool atapi = false,
			uint pmp = 0,uint features = 0);
	void add_dma(const nre::DataSpace &ds,size_t offset,uint count);
	void add_prd(const nre::DataSpace &ds,uint count);
	size_t start_command(nre::Producer<nre::Storage::Packet> *prod,ulong usertag);
	void identify_drive(nre::DataSpace &buffer);
	uint set_features(uint features,uint count = 0);

	nre::UserSm _sm;
	Register volatile *_regs;
	nre::Clock _clock;
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
	UserTag _usertags[32];
	uint _inprogress;
};
