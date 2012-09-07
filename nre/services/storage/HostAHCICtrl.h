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

#include <kobj/Gsi.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>
#include <util/PCI.h>
#include <Assert.h>
#include <CPU.h>

#include "HostAHCIDevice.h"
#include "Controller.h"

/**
 * A simple driver for AHCI.
 *
 * State: testing
 * Features: Ports
 */
class HostAHCICtrl : public Controller {

	/**
	 * The register set of an AHCI controller.
	 */
	struct Register {
		union {
			struct {
				volatile uint32_t cap;
				volatile uint32_t ghc;
				volatile uint32_t is;
				volatile uint32_t pi;
				volatile uint32_t vs;
				volatile uint32_t ccc_ctl;
				volatile uint32_t ccc_ports;
				volatile uint32_t em_loc;
				volatile uint32_t em_ctl;
				volatile uint32_t cap2;
				volatile uint32_t bohc;
			};
			volatile uint32_t generic[0x100 >> 2];
		};
		HostAHCIDevice::Register ports[32];
	};

public:
	explicit HostAHCICtrl(nre::PCI &pci,nre::PCI::bdf_type bdf,nre::Gsi *gsi,bool dmar)
			: Controller(), _gsi(gsi), _gsigt(gsi_thread,nre::CPU::current().log_id()),
			  _gsisc(&_gsigt,nre::Qpd()), _bdf(bdf), _regs_ds(), _regs_high_ds(), _regs(),
			  _regs_high(0), _portcount(0), _ports() {
		assert(!(~pci.conf_read(_bdf,1) & 6) && "we need mem-decode and busmaster dma");
		nre::PCI::value_type bar = pci.conf_read(_bdf,9);
		assert(!(bar & 7) && "we need a 32bit memory bar");

		_regs_ds = new nre::DataSpace(0x1000,
				nre::DataSpaceDesc::LOCKED,nre::DataSpaceDesc::RW,bar);
		_regs = reinterpret_cast<Register*>(_regs_ds->virt() + (bar & 0xFFF));

		// map the high ports
		if(_regs->pi >> 30) {
			_regs_high_ds = new nre::DataSpace(0x1000,
					nre::DataSpaceDesc::LOCKED,nre::DataSpaceDesc::RW,bar + 0x1000);
			_regs_high = reinterpret_cast<HostAHCIDevice::Register*>(
					_regs_high_ds->virt() + (bar & 0xfe0));
		}

		// enable AHCI
		_regs->ghc |= 0x80000000;
		LOG(nre::Logging::STORAGE,nre::Serial::get().writef(
				"AHCI: cap %#x cap2 %#x global %#x ports %#x version %#x bohc %#x\n",
				_regs->cap,_regs->cap2,_regs->ghc,_regs->pi,_regs->vs,_regs->bohc));
		assert(!_regs->bohc);

		// create ports
		memset(_ports,0,sizeof(_ports));
		for(uint i = 0; i < 30; i++)
			create_ahci_port(i,_regs->ports + i,dmar);
		for(uint i = 30; _regs_high && i < 32; i++)
			create_ahci_port(i,_regs_high + (i - 30),dmar);

		// clear pending irqs
		_regs->is = _regs->pi;
		// enable IRQs
		_regs->ghc |= 2;

		// start the gsi thread
		_gsigt.set_tls<HostAHCICtrl*>(nre::Thread::TLS_PARAM,this);
		char name[32];
		nre::OStringStream os(name,sizeof(name));
		os << "ahci-gsi-" << _gsi->gsi();
		_gsisc.start(nre::String(name));
	}
	virtual ~HostAHCICtrl() {
		delete _gsi;
		delete _regs_ds;
		delete _regs_high_ds;
	}

	virtual bool exists(size_t drive) const {
		return drive < ARRAY_SIZE(_ports) && _ports[drive] != 0;
	}
	virtual size_t drive_count() const {
		return _portcount;
	}

	virtual void get_params(size_t drive,nre::Storage::Parameter *params) const {
		assert(_ports[drive]);
		_ports[drive]->get_params(params);
	}

	virtual void flush(size_t drive,producer_type *prod,tag_type tag) {
		assert(_ports[drive]);
		_ports[drive]->flush(prod,tag);
	}

	virtual void read(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes) {
		assert(_ports[drive]);
		_ports[drive]->readwrite(prod,tag,ds,sector,offset,bytes,false);
	}
	virtual void write(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes) {
		assert(_ports[drive]);
		_ports[drive]->readwrite(prod,tag,ds,sector,offset,bytes,true);
	}

private:
	void create_ahci_port(uint nr,HostAHCIDevice::Register *portreg,bool dmar) {
		// port not implemented
		if(!(_regs->pi & (1 << nr)))
			return;

		// check what kind of drive we have, if any
		uint32_t sig = HostAHCIDevice::get_signature(portreg);
		if(sig != HostAHCIDevice::SATA_SIG_NONE) {
			_ports[nr] = new HostAHCIDevice(portreg,_portcount,((_regs->cap >> 8) & 0x1f) + 1,dmar);
			_ports[nr]->determine_capacity();
			LOG(nre::Logging::STORAGE,_ports[nr]->print());
			_portcount++;
		}
	}

	static void gsi_thread(void*) {
		HostAHCICtrl *ha = nre::Thread::current()->get_tls<HostAHCICtrl*>(nre::Thread::TLS_PARAM);
		while(1) {
			ha->_gsi->down();

			uint32_t is = ha->_regs->is;
			uint32_t oldis = is;
			while(is) {
				uint32_t port = nre::Math::bit_scan_forward(is);
				if(ha->_ports[port])
					ha->_ports[port]->irq();
				is &= ~(1 << port);
			}
			ha->_regs->is = oldis;
		}
	}

	nre::Gsi *_gsi;
	nre::GlobalThread _gsigt;
	nre::Sc _gsisc;
	nre::PCI::bdf_type _bdf;
	uint _hostirq;
	nre::DataSpace *_regs_ds;
	nre::DataSpace *_regs_high_ds;
	Register *_regs;
	HostAHCIDevice::Register *_regs_high;
	size_t _portcount;
	HostAHCIDevice *_ports[32];
};
