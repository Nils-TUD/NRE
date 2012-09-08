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

#include "HostATADevice.h"

class HostATAPIDevice : public HostATADevice {
public:
	explicit HostATAPIDevice(HostIDECtrl &ctrl,uint id,const Identify &info)
		: HostATADevice(ctrl,id,info),
		  _cmd(nre::ExecEnv::PAGE_SIZE,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW) {
	}

	virtual void determine_capacity();
	virtual void readwrite(Operation op,const nre::DataSpace &ds,size_t offset,sector_type sector,
			sector_type count,size_t secsize = 0);

private:
	uint8_t *cmdaddr() const {
		return reinterpret_cast<uint8_t*>(_cmd.virt());
	}
	void request(const nre::DataSpace &cmd,const nre::DataSpace &data,size_t offset,size_t bufSize);

	nre::DataSpace _cmd;
};

