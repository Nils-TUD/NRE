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

#include <Compiler.h>
#include <Logging.h>

#include "Device.h"
#include "HostIDECtrl.h"

class HostATADevice : public Device {
public:
	enum Operation {
		READ,
		WRITE,
		PACKET
	};

	explicit HostATADevice(HostIDECtrl &ctrl,uint id,const Identify &info)
		: Device(id,info), _ctrl(ctrl) {
	}
	virtual ~HostATADevice() {
	}

	virtual const char *type() const {
		return is_atapi() ? "ATAPI" : "ATA";
	}
	virtual void determine_capacity() {
		_capacity = has_lba48() ? _info.lba48MaxLBA : _info.userSectorCount;
	}
	virtual void readwrite(Operation op,const nre::DataSpace &ds,size_t offset,sector_type sector,
			sector_type count,size_t secsize = 0);
	void flush_cache();

protected:
	void transferPIO(Operation op,const nre::DataSpace &ds,size_t offset,size_t secsize,
			sector_type count,bool waitfirst);
	void transferDMA(Operation op,const nre::DataSpace &ds,size_t offset,size_t secsize,
			sector_type count);

	uint get_command(Operation op);
	void setup_command(sector_type sector,sector_type count,uint cmd);

	HostIDECtrl &_ctrl;
};
