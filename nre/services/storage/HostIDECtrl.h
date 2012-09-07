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

#include <kobj/Ports.h>
#include <kobj/Gsi.h>
#include <util/Clock.h>

#include "Device.h"
#include "Controller.h"

class HostATADevice;

class HostIDECtrl : public Controller {
	enum {
		DEVICE_PRIMARY		= 0,
		DEVICE_SECONDARY	= 1
	};
	enum {
		DEVICE_COUNT		= 4
	};
	enum {
		DEVICE_PRIM_MASTER	= 0,
		DEVICE_PRIM_SLAVE	= 1,
		DEVICE_SEC_MASTER	= 2,
		DEVICE_SEC_SLAVE	= 3,
	};

public:
	explicit HostIDECtrl(uint id,uint irq,nre::Ports::port_t portbase,nre::Ports::port_t bmportbase,
			uint bmportcount,bool dma = true);
	virtual ~HostIDECtrl() {
		delete _bm;
	}

	virtual size_t drive_count() const {
		return (_devs[0] ? 1 : 0) + (_devs[1] ? 1 : 0);
	}

	virtual bool exists(size_t drive) const {
		return drive < 2 && _devs[drive];
	}

	virtual void get_params(size_t drive,nre::Storage::Parameter *params) const;

	virtual void flush(size_t drive,producer_type *prod,tag_type tag) {
		// TODO
#if 0
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		uint8_t packets[18];
		HostATA &params = _params[drive];
		// XXX handle RO media
		make_sector_packets(params,packets,params._lba48 ? 0xea : 0xe7,0,0);
		uint res;
		if((res = ata_command(packets + 8,0,0,true)))
			throw nre::Exception(nre::E_FAILURE,32,"ATA command failed with %#x",res);
		prod->produce(nre::Storage::Packet(tag,0));
#endif
	}

	virtual void read(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes);
	virtual void write(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes);

	bool dma_enabled() const {
		return _dma;
	}
	bool irqs_enabled() const {
		return _irqs;
	}

	/**
	 * Waits for <set> to set and <unset> to unset in the status-register.
	 * It gives up as soon as <timeout> is reached.
	 *
	 * @return 0 on success, -1 if timeout has been reached, other: value of the error-register
	 */
	int wait_until(timevalue_t timeout,uint8_t set,uint8_t unset);

	/**
	 * Performs a few io-port-reads (just to waste a bit of time ;))
	 */
	void wait() {
		inb(ATA_REG_STATUS);
		inb(ATA_REG_STATUS);
		inb(ATA_REG_STATUS);
		inb(ATA_REG_STATUS);
	}

	/**
	 * Writes <value> to the bus-master-register <reg> of the given controller
	 */
	void outbmrb(uint16_t reg,uint8_t value) {
		_bm->out<uint8_t>(value,reg);
	}
	void outbmrl(uint16_t reg,uint32_t value) {
		_bm->out<uint32_t>(value,reg);
	}

	/**
	 * Reads a byte from the bus-master-register <reg> of the given controller
	 */
	uint8_t inbmrb(uint16_t reg) {
		return _bm->in<uint8_t>(reg);
	}

	/**
	 * Writes <value> to the controller-register <reg>
	 */
	void outb(uint16_t reg,uint8_t value) {
		_ctrl.out<uint8_t>(value,reg);
	}
	/**
	 * Writes <value> to the control-register
	 */
	void ctrloutb(uint8_t value) {
		_ctrlreg.out<uint8_t>(value);
	}
	/**
	 * Writes <count> words from <buf> to the controller-register <reg>
	 */
	void outwords(uint16_t reg,const uint16_t *buf,size_t count) {
		size_t i;
		for(i = 0; i < count; i++)
			_ctrl.out<uint16_t>(buf[i],reg);
	}

	/**
	 * Reads a byte from the controller-register <reg>
	 */
	uint8_t inb(uint16_t reg) {
		return _ctrl.in<uint8_t>(reg);
	}

	/**
	 * Reads <count> words from the controller-register <reg> into <buf>
	 */
	void inwords(uint16_t reg,uint16_t *buf,size_t count) {
		size_t i;
		for(i = 0; i < count; i++)
			buf[i] = _ctrl.in<uint16_t>(reg);
	}

private:
	bool is_bus_responding();
	HostATADevice *detect_drive(uint id);
	HostATADevice *identify(uint id,uint cmd);

	static void gsi_thread(void *arg) {
		uint no = reinterpret_cast<word_t>(arg);
		nre::Gsi gsi(no);
		while(1) {
			gsi.down();
		}
	}

	uint _id;
	bool _dma;
	bool _irqs;
	nre::Ports _ctrl;
	nre::Ports _ctrlreg;
	nre::Ports *_bm;
	nre::Clock _clock;
	nre::UserSm _sm;
	HostATADevice *_devs[2];
};
