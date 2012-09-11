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
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <util/Clock.h>
#include <Logging.h>

#include "Device.h"
#include "Controller.h"

class HostATADevice;

class HostIDECtrl : public Controller {
	struct UserTag {
		nre::Producer<nre::Storage::Packet> *prod;
		nre::Storage::tag_type tag;
		bool dma;
	};

public:
	/* physical region descriptor */
	struct PRD {
		uint32_t buffer;
		uint16_t byteCount;
		uint16_t : 15;
		uint16_t last : 1;
	} PACKED;

	explicit HostIDECtrl(uint id,uint irq,nre::Ports::port_t portbase,
			nre::Ports::port_t bmportbase,uint bmportcount,bool dma = true);
	virtual ~HostIDECtrl() {
		delete _bm;
	}

	virtual size_t drive_count() const {
		return (_devs[0] ? 1 : 0) + (_devs[1] ? 1 : 0);
	}
	virtual bool exists(size_t drive) const {
		return idx(drive) < ARRAY_SIZE(_devs) && _devs[idx(drive)];
	}

	virtual void get_params(size_t drive,nre::Storage::Parameter *params) const;
	virtual void flush(size_t drive,producer_type *prod,tag_type tag);
	virtual void read(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,const dma_type &dma);
	virtual void write(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,const dma_type &dma);

	/**
	 * @return whether DMA should and can be used
	 */
	bool dma_enabled() const {
		return _dma;
	}
	/**
	 * @return whether interrupts should be used
	 */
	bool irqs_enabled() const {
		return _irqs;
	}

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
	 * Stores that we're waiting for the result of a transfer. You should do that before actually
	 * starting the transfer.
	 */
	void start_transfer(producer_type *prod,tag_type tag,bool dma) {
		_tag.prod = prod;
		_tag.tag = tag;
		_tag.dma = dma;
		_in_progress = true;
	}

	/**
	 * Stores that we're not waiting anymore. So, it assumes that the transfer is finished.
	 */
	void stop_transfer() {
		_in_progress = false;
	}

	/**
	 * Waits until a new transfer can be started, if necessary
	 */
	void wait_ready() {
		if(_irqs && _in_progress)
			_ready.zero();
	}

	/**
	 * Waits for <set> to set and <unset> to unset in the status-register.
	 * It gives up as soon as <timeout> is reached.
	 *
	 * @return 0 on success, -1 if timeout has been reached, other: value of the error-register
	 */
	int wait_until(timevalue_t timeout,uint8_t set,uint8_t unset);

	/**
	 * Checks whether <res> is OK and throws exceptions with corresponding messages if not
	 *
	 * @param device the device-id
	 * @param res the result from reading the status register
	 * @param name the name of the operation to display
	 */
	void handle_status(uint device,int res,const char *name);

	/**
	 * @return pointer to the PRDT (virtual)
	 */
	PRD *prdt() const {
		return reinterpret_cast<PRD*>(_prdt.virt());
	}
	/**
	 * @return physical address of PRDT
	 */
	uintptr_t prdt_addr() const {
		return _prdt.phys();
	}

	/**
	 * Reads a byte from the bus-master-register <reg> of the given controller
	 */
	uint8_t inbmrb(uint16_t reg) {
		return _bm->in<uint8_t>(reg);
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

private:
	static size_t idx(size_t drive) {
		return drive % nre::Storage::MAX_DRIVES;
	}
	bool is_bus_responding();
	HostATADevice *detect_drive(uint id);
	HostATADevice *identify(uint id,uint cmd);

	static void gsi_thread(void *) {
		HostIDECtrl *ctrl = nre::Thread::current()->get_tls<HostIDECtrl*>(nre::Thread::TLS_PARAM);
		while(1) {
			ctrl->_gsi->down();

			LOG(nre::Logging::STORAGE_DETAIL,nre::Serial::get() << "Got GSI " << ctrl->_gsi->gsi() << "\n");
			uint status = 0;
			if(ctrl->_tag.dma) {
				int res = ctrl->wait_until(DMA_TRANSFER_TIMEOUT,0,CMD_ST_BUSY | CMD_ST_DRQ);
				if(res != 0)
					status = 1;

				ctrl->inbmrb(BMR_REG_STATUS);
				ctrl->outbmrb(BMR_REG_COMMAND,0);
			}
			if(ctrl->_tag.prod)
				ctrl->_tag.prod->produce(nre::Storage::Packet(ctrl->_tag.tag,status));
			ctrl->_ready.up();
			ctrl->_in_progress = false;
			// just in case we receive another interrupt
			// TODO this is not enough. we have to use the session-id and check whether the session
			// still exists. because the user might have closed it between a transfer-start and
			// the completion
			ctrl->_tag.prod = 0;
			ctrl->_tag.dma = false;
		}
	}

	bool _dma;
	bool _irqs;
	bool _in_progress;
	nre::Sm _ready;
	nre::Ports _ctrl;
	nre::Ports _ctrlreg;
	nre::Ports *_bm;
	nre::Clock _clock;
	nre::UserSm _sm;
	nre::Gsi *_gsi;
	nre::DataSpace _prdt;
	nre::GlobalThread _gsigt;
	nre::Sc _gsisc;
	UserTag _tag;
	HostATADevice *_devs[2];
};
