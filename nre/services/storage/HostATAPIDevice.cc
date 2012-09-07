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

bool HostATAPIDevice::readwrite(Operation op,const DataSpace &ds,size_t offset,uint64_t sector,
		size_t count,size_t secsize) {
	if(secsize == 0)
		secsize = _sector_size;
	uint8_t *cmd = cmdaddr();
	memset(cmd,0,12);
	cmd[0] = SCSI_CMD_READ_SECTORS_EXT;
	if(!has_lba48())
		cmd[0] = SCSI_CMD_READ_SECTORS;
	/* no writing here ;) */
	if(op != READ)
		return false;
	if(count == 0)
		return false;
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
    return request(_cmd,ds,offset,count * _sector_size);
}

void HostATAPIDevice::determine_capacity() {
	DataSpace ds(8 + 12,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	uint8_t *cmd = reinterpret_cast<uint8_t*>(ds.virt());
	uint8_t *resp = cmd + 8;
	memset(cmd,0,20);
	cmd[0] = SCSI_CMD_READ_CAPACITY;
	bool res = request(ds,ds,8,8);
	if(!res)
		_capacity = 0;
	else
		_capacity = (resp[0] << 24) | (resp[1] << 16) | (resp[2] << 8) | (resp[3] << 0);
}

bool HostATAPIDevice::request(const DataSpace &cmd,const DataSpace &data,size_t offset,size_t bufSize) {
	int res;
	size_t size;

	/* send PACKET command to drive */
	if(!HostATADevice::readwrite(PACKET,cmd,0,0xFFFF00,1,12))
		return false;

	/* now transfer the data */
	if(_ctrl.dma_enabled() && has_dma())
		return transferDMA(READ,data,offset,_sector_size,bufSize / _sector_size);

	/* ok, no DMA, so wait first until the drive is ready */
	res = _ctrl.wait_until(ATAPI_TRANSFER_TIMEOUT,CMD_ST_DRQ,CMD_ST_BUSY);
	if(res == -1) {
		ATA_LOG("Device %d: Timeout before ATAPI-PIO-transfer",_id);
		return false;
	}
	if(res != 0) {
		ATA_LOG("Device %d: ATAPI-PIO-transfer failed: %#x",_id,res);
		return false;
	}

	/* read the actual size per transfer */
	ATA_LOGDETAIL("Reading response-size");
	size = ((size_t)_ctrl.inb(ATA_REG_ADDRESS3) << 8) | (size_t)_ctrl.inb(ATA_REG_ADDRESS2);
	/* do the PIO-transfer (no check at the beginning; seems to cause trouble on some machines) */
	return transferPIO(READ,data,offset,size,bufSize / size,false);
}
