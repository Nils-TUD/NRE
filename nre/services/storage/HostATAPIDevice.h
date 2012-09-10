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
		: HostATADevice(ctrl,id,info) {
	}

	virtual void determine_capacity();
	virtual void readwrite(Operation op,const nre::DataSpace &ds,sector_type sector,
			const dma_type &dma,size_t secsize = 0);

private:
	void request(const nre::DataSpace &cmd,const nre::DataSpace &data,const dma_type &dma);
};

