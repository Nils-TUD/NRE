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

#include "HostIDECtrl.h"
#include "HostATADevice.h"
#include "HostATAPIDevice.h"

using namespace nre;

/* for some reason virtualbox requires an additional port (9 instead of 8). Otherwise
 * we are not able to access port (portbase + 7). */
HostIDECtrl::HostIDECtrl(uint id,uint gsi,Ports::port_t portbase,
		Ports::port_t bmportbase,uint bmportcount,bool dma)
		: _id(id), _dma(dma && bmportbase), _irqs(true), _ctrl(portbase,9),
		  _ctrlreg(portbase + ATA_REG_CONTROL,1),
		  _bm(dma && bmportbase ? new Ports(bmportbase,bmportcount) : 0), _clock(1000), _sm(),
		  _gsi(gsi ? new Gsi(gsi) : 0), _prdt(8,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
		  _gsigt(gsi_thread,CPU::current().log_id()), _gsisc(&_gsigt,Qpd()), _devs() {
	// check if the bus is empty
	if(!is_bus_responding())
		throw Exception(E_NOT_FOUND,32,"Bus %u is floating",_id);

	// init attached drives; begin with slave
	for(ssize_t j = 1; j >= 0; --j) {
		try {
			_devs[j] = detect_drive(_id * 2 + j);
			LOG(nre::Logging::STORAGE,_devs[j]->print());
		}
		catch(const Exception &e) {
			ATA_LOGDETAIL("Identify failed: %s",e.msg());
			_devs[j] = 0;
		}
	}

#if 0
	if(irq != 0) {
		char name[32];
		nre::OStringStream os(name,sizeof(name));
		os << "ide-gsi-" << irq;
		_gsigt.set_tls<HostIDECtrl*>(Thread::TLS_PARAM,this);
		_gsisc.start(nre::String(name));
	}
#endif
}

void HostIDECtrl::get_params(size_t drive,nre::Storage::Parameter *params) const {
	_devs[drive]->get_params(params);
}

void HostIDECtrl::read(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
		size_t offset,sector_type sector,sector_type count) {
	nre::ScopedLock<nre::UserSm> guard(&_sm);
	_devs[drive]->readwrite(HostATADevice::READ,ds,offset,sector,count);
	// TODO wrong place
	prod->produce(nre::Storage::Packet(tag,0));
}

void HostIDECtrl::write(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
		size_t offset,sector_type sector,sector_type count) {
	nre::ScopedLock<nre::UserSm> guard(&_sm);
	_devs[drive]->readwrite(HostATADevice::WRITE,ds,offset,sector,count);
	// TODO wrong place
	prod->produce(nre::Storage::Packet(tag,0));
}

void HostIDECtrl::flush(size_t drive,producer_type *prod,tag_type tag) {
	nre::ScopedLock<nre::UserSm> guard(&_sm);
	_devs[drive]->flush_cache();
	// TODO wrong place
	prod->produce(nre::Storage::Packet(tag,0));
}

HostATADevice *HostIDECtrl::detect_drive(uint id) {
	HostATADevice *dev;
	try {
		dev = identify(id,COMMAND_IDENTIFY);
	}
	catch(const Exception &e) {
		ATA_LOGDETAIL("Identify failed (%s), trying IDENTIFY_PACKET now",e.msg());
		dev = identify(id,COMMAND_IDENTIFY_PACKET);
	}

	dev->determine_capacity();
	return dev;
}

HostATADevice *HostIDECtrl::identify(uint id,uint cmd) {
	outb(ATA_REG_DRIVE_SELECT,(id & SLAVE_BIT) << 4);
	wait();

	// disable interrupts
	ctrloutb(CTRL_NIEN);

	// check whether the drive exists
	outb(ATA_REG_COMMAND,cmd);
	uint8_t status = inb(ATA_REG_STATUS);
	if(status == 0)
		throw Exception(E_NOT_FOUND,64,"Device %u: Got 0 from status-port. Assuming its not present",id);

	// TODO from the osdev-wiki: Because of some ATAPI drives that do not follow spec, at this point
	// you need to check the LBAmid and LBAhi ports (0x1F4 and 0x1F5) to see if they are
	// non-zero. If so, the drive is not ATA, and you should stop polling.

	// wait while busy; the other bits aren't valid while busy is set
	timevalue_t until = _clock.source_time(ATA_WAIT_TIMEOUT);
	while((inb(ATA_REG_STATUS) & CMD_ST_BUSY) && _clock.source_time() < until)
		Util::pause();
	// wait a bit
	wait();

	// wait until ready (or error)
	timevalue_t timeout = cmd == COMMAND_IDENTIFY_PACKET ? ATAPI_WAIT_TIMEOUT : ATA_WAIT_TIMEOUT;
	int res = wait_until(timeout,CMD_ST_DRQ,CMD_ST_BUSY);
	if(res == -1)
		throw Exception(E_NOT_FOUND,64,"Device %u: Timeout reached while waiting on ready",id);
	if(res != 0)
		throw Exception(E_NOT_FOUND,64,"Device %u: Error %#x. Assuming its not present",id,res);

	// drive is ready, read data
	HostATADevice::Identify info;
	inwords(ATA_REG_DATA,reinterpret_cast<uint16_t*>(&info),256);

	// wait until DRQ and BUSY bits are unset
	res = wait_until(ATA_WAIT_TIMEOUT,0,CMD_ST_DRQ | CMD_ST_BUSY);
	if(res == -1)
		throw Exception(E_NOT_FOUND,64,"Device %u: Timeout reached while waiting DRQ bit to clear",id);
	if(res != 0)
		throw Exception(E_NOT_FOUND,64,"Device %u: Error %#x. Assuming its not present",id);

	// we don't support CHS atm
	if(info.capabilities.LBA == 0)
		throw Exception(E_NOT_FOUND,64,"Device %u: LBA not supported",id);

	if(info.general.isATAPI)
		return new HostATAPIDevice(*this,id,info);
	return new HostATADevice(*this,id,info);
}

bool HostIDECtrl::is_bus_responding() {
	for(ssize_t i = 1; i >= 0; i--) {
		// begin with slave. master should respond if there is no slave
		outb(ATA_REG_DRIVE_SELECT,i << 4);
		wait();

		// write some arbitrary values to some registers
		outb(ATA_REG_ADDRESS1,0xF1);
		outb(ATA_REG_ADDRESS2,0xF2);
		outb(ATA_REG_ADDRESS3,0xF3);

		// if we can read them back, the bus is present
		if(inb(ATA_REG_ADDRESS1) == 0xF1 && inb(ATA_REG_ADDRESS2) == 0xF2 && inb(ATA_REG_ADDRESS3) == 0xF3)
			return true;
	}
	return false;
}

int HostIDECtrl::wait_until(timevalue_t timeout,uint8_t set,uint8_t unset) {
	timevalue_t until = _clock.source_time(timeout);
	while(_clock.source_time() < until) {
		uint8_t status = inb(ATA_REG_STATUS);
		if(status & CMD_ST_ERROR)
			return inb(ATA_REG_ERROR);
		if((status & set) == set && !(status & unset))
			return 0;
		Util::pause();
	}
	return -1;
}

void HostIDECtrl::handle_status(uint device,int res,const char *name) {
	if(res == -1)
		throw Exception(E_TIMEOUT,32,"Device %u: Timeout during %s",device,name);
	if(res != 0)
		throw Exception(E_FAILURE,32,"Device %u: %s failed: %#x",device,name,res);
}
