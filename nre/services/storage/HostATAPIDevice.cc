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

#include "HostATAPIDevice.h"

using namespace nre;

void HostATAPIDevice::readwrite(Operation op,const DataSpace &ds,sector_type sector,
		const dma_type &dma,size_t secsize) {
	if(secsize == 0)
		secsize = _sector_size;
	size_t count = dma.bytecount() / _sector_size;
	uint8_t *cmd = buffer();
	memset(cmd,0,12);
	cmd[0] = SCSI_CMD_READ_SECTORS_EXT;
	if(!has_lba48())
		cmd[0] = SCSI_CMD_READ_SECTORS;
	/* no writing here ;) */
	if(op != READ)
		throw Exception(E_ARGS_INVALID,64,"Device %u: Writing is not supported for ATAPI",_id);
	if(cmd[0] == SCSI_CMD_READ_SECTORS_EXT) {
		cmd[6] = (count >> 24) & 0xFF;
		cmd[7] = (count >> 16) & 0xFF;
		cmd[8] = (count >> 8) & 0xFF;
		cmd[9] = (count >> 0) & 0xFF;
	}
	else {
		cmd[7] = (count >> 8) & 0xFF;
		cmd[8] = (count >> 0) & 0xFF;
	}
	cmd[2] = (sector >> 24) & 0xFF;
	cmd[3] = (sector >> 16) & 0xFF;
	cmd[4] = (sector >> 8) & 0xFF;
	cmd[5] = (sector >> 0) & 0xFF;
	request(_buffer,ds,dma);
}

void HostATAPIDevice::determine_capacity() {
	dma_type dma;
	dma.add(DMADesc(8,8));
	uint8_t *cmd = buffer();
	uint8_t *resp = cmd + 8;
	memset(cmd,0,20);
	cmd[0] = SCSI_CMD_READ_CAPACITY;
	request(_buffer,_buffer,dma);
	_capacity = (resp[0] << 24) | (resp[1] << 16) | (resp[2] << 8) | (resp[3] << 0);
}

void HostATAPIDevice::request(const DataSpace &cmd,const DataSpace &data,const dma_type &dma) {
	/* send PACKET command to drive */
	dma_type cdma;
	cdma.add(DMADesc(0,12));
	HostATADevice::readwrite(PACKET,cmd,0xFFFF00,cdma,12);

	/* now transfer the data */
	if(_ctrl.dma_enabled() && has_dma()) {
		transferDMA(READ,data,dma);
		return;
	}

	/* ok, no DMA, so wait first until the drive is ready */
	int res = _ctrl.wait_until(ATAPI_TRANSFER_TIMEOUT,CMD_ST_DRQ,CMD_ST_BUSY);
	_ctrl.handle_status(_id,res,"ATAPI PIO transfer");

	/* read the actual size per transfer */
	ATA_LOGDETAIL("Reading response-size");
	size_t size = ((size_t)_ctrl.inb(ATA_REG_ADDRESS3) << 8) | (size_t)_ctrl.inb(ATA_REG_ADDRESS2);
	/* do the PIO-transfer (no check at the beginning; seems to cause trouble on some machines) */
	transferPIO(READ,data,size,dma,false);
}
