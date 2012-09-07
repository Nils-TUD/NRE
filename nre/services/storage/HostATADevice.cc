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

#include "HostATADevice.h"

using namespace nre;

uint HostATADevice::get_command(Operation op) {
	static uint commands[4][2] = {
		{COMMAND_READ_SEC,COMMAND_READ_SEC_EXT},
		{COMMAND_WRITE_SEC,COMMAND_WRITE_SEC_EXT},
		{COMMAND_READ_DMA,COMMAND_READ_DMA_EXT},
		{COMMAND_WRITE_DMA,COMMAND_WRITE_DMA_EXT}
	};
	uint offset;
	if(op == PACKET)
		return COMMAND_PACKET;
	offset = (_ctrl.dma_enabled() && has_dma()) ? 2 : 0;
	if(op == WRITE)
		offset++;
	return commands[offset][has_lba48() ? 1 : 0];
}

bool HostATADevice::readwrite(Operation op,const DataSpace &ds,size_t offset,uint64_t sector,size_t count,size_t secsize) {
	if(secsize == 0)
		secsize = _sector_size;
	uint cmd = get_command(op);
	if(!setup_command(sector,count,cmd))
		return false;

	switch(cmd) {
		case COMMAND_PACKET:
		case COMMAND_READ_SEC:
		case COMMAND_READ_SEC_EXT:
		case COMMAND_WRITE_SEC:
		case COMMAND_WRITE_SEC_EXT:
			return transferPIO(op,ds,offset,secsize,count,true);
		case COMMAND_READ_DMA:
		case COMMAND_READ_DMA_EXT:
		case COMMAND_WRITE_DMA:
		case COMMAND_WRITE_DMA_EXT:
			return transferDMA(op,ds,offset,secsize,count);
	}
	return false;
}

bool HostATADevice::transferPIO(Operation op,const DataSpace &ds,size_t offset,size_t secsize,size_t count,bool waitfirst) {
	size_t i;
	int res;
	uint16_t *buf = reinterpret_cast<uint16_t*>(ds.virt() + offset);
	for(i = 0; i < count; i++) {
		if(i > 0 || waitfirst) {
			/* TODO if(op == READ)
				ctrl_waitIntrpt(ctrl);*/
			res = _ctrl.wait_until(PIO_TRANSFER_TIMEOUT,CMD_ST_DRQ,CMD_ST_BUSY);
			if(res == -1) {
				ATA_LOG("Device %d: Timeout before PIO-transfer",_id);
				return false;
			}
			if(res != 0) {
				/* TODO ctrl_softReset(ctrl);*/
				ATA_LOG("Device %d: PIO-transfer failed: %#x",_id,res);
				return false;
			}
		}

		/* now read / write the data */
		ATA_LOGDETAIL("Ready, starting read/write");
		if(op == READ)
			_ctrl.inwords(ATA_REG_DATA,buf,secsize / sizeof(uint16_t));
		else
			_ctrl.outwords(ATA_REG_DATA,buf,secsize / sizeof(uint16_t));
		buf += secsize / sizeof(uint16_t);
		ATA_LOGDETAIL("Transfer done");
	}
	ATA_LOGDETAIL("All sectors done");
	return true;
}

bool HostATADevice::transferDMA(Operation op,const DataSpace &ds,size_t offset,size_t secSize,size_t secCount) {
#if 0
	uint8_t status;
	size_t size = secCount * secSize;
	int res;

	/* setup PRDT */
	ctrl->dma_prdt_virt->buffer = ctrl->dma_buf_phys;
	ctrl->dma_prdt_virt->byteCount = size;
	ctrl->dma_prdt_virt->last = 1;

	/* stop running transfers */
	ATA_LOGDETAIL("Stopping running transfers");
	_ctrl.outbmrb(BMR_REG_COMMAND,0);
	status = _ctrl.inbmrb(BMR_REG_STATUS) | BMR_STATUS_ERROR | BMR_STATUS_IRQ;
	_ctrl.outbmrb(BMR_REG_STATUS,status);

	/* set PRDT */
	ATA_LOGDETAIL("Setting PRDT");
	_ctrl.outbmrl(BMR_REG_PRDT,(uint32_t)ctrl->dma_prdt_phys);

	/* write data to buffer, if we should write */
	/* TODO we should use the buffer directly when reading from the client */
	if(op == WRITE || op == PACKET)
		memcpy(ctrl->dma_buf_virt,buffer,size);

	/* it seems to be necessary to read those ports here */
	ATA_LOGDETAIL("Starting DMA-transfer");
	_ctrl.inbmrb(BMR_REG_COMMAND);
	_ctrl.inbmrb(BMR_REG_STATUS);
    /* start bus-mastering */
	if(op == READ)
		_ctrl.outbmrb(BMR_REG_COMMAND,BMR_CMD_START | BMR_CMD_READ);
	else
		_ctrl.outbmrb(BMR_REG_COMMAND,BMR_CMD_START);
	_ctrl.inbmrb(BMR_REG_COMMAND);
	_ctrl.inbmrb(BMR_REG_STATUS);

	/* now wait for an interrupt */
	ATA_LOGDETAIL("Waiting for an interrupt");
	ctrl_waitIntrpt(ctrl);

	res = _ctrl.wait_until(DMA_TRANSFER_TIMEOUT,DMA_TRANSFER_SLEEPTIME,0,CMD_ST_BUSY | CMD_ST_DRQ);
	if(res == -1) {
		ATA_LOG("Device %d: Timeout after DMA-transfer",_id);
		return false;
	}
	if(res != 0) {
		ATA_LOG("Device %d: DMA-Transfer failed: %#x",_id,res);
		return false;
	}

	_ctrl.inbmrb(BMR_REG_STATUS);
	_ctrl.outbmrb(BMR_REG_COMMAND,0);
	/* copy data when reading */
	if(op == READ)
		memcpy(buffer,ctrl->dma_buf_virt,size);
#endif
	return true;
}

bool HostATADevice::setup_command(uint64_t lba,size_t secCount,uint cmd) {
	uint8_t devValue;
	if(secCount == 0)
		return false;

	if(!has_lba48()) {
		if(lba & 0xFFFFFFFFF0000000LL) {
			ATA_LOG("Trying to read from %#Lx with LBA28",lba);
			return false;
		}
		if(secCount & 0xFF00) {
			ATA_LOG("Trying to read %u sectors with LBA28",secCount);
			return false;
		}

		/* For LBA28, the lowest 4 bits are bits 27-24 of LBA */
		devValue = DEVICE_LBA | ((_id & SLAVE_BIT) << 4) | ((lba >> 24) & 0x0F);
		_ctrl.outb(ATA_REG_DRIVE_SELECT,devValue);
	}
	else {
		devValue = DEVICE_LBA | ((_id & SLAVE_BIT) << 4);
		_ctrl.outb(ATA_REG_DRIVE_SELECT,devValue);
	}

	ATA_LOGDETAIL("Selecting device %d (%s)",_id,is_atapi() ? "ATAPI" : "ATA");
	_ctrl.wait();

	ATA_LOGDETAIL("Resetting control-register");
	// TODO ctrl_resetIrq(ctrl);
	/* reset control-register */
	_ctrl.ctrloutb(_ctrl.irqs_enabled() ? 0 : CTRL_NIEN);

	/* needed for ATAPI */
	if(is_atapi())
		_ctrl.outb(ATA_REG_FEATURES,_ctrl.dma_enabled() && has_dma());

	if(has_lba48()) {
		ATA_LOGDETAIL("LBA48: setting sector-count %d and LBA %x",secCount,(uint)(lba & 0xFFFFFFFF));
		/* LBA: | LBA6 | LBA5 | LBA4 | LBA3 | LBA2 | LBA1 | */
		/*     48             32            16            0 */
		/* sector-count high-byte */
		_ctrl.outb(ATA_REG_SECTOR_COUNT,(uint8_t)(secCount >> 8));
		/* LBA4, LBA5 and LBA6 */
		_ctrl.outb(ATA_REG_ADDRESS1,(uint8_t)(lba >> 24));
		_ctrl.outb(ATA_REG_ADDRESS2,(uint8_t)(lba >> 32));
		_ctrl.outb(ATA_REG_ADDRESS3,(uint8_t)(lba >> 40));
		/* sector-count low-byte */
		_ctrl.outb(ATA_REG_SECTOR_COUNT,(uint8_t)(secCount & 0xFF));
		/* LBA1, LBA2 and LBA3 */
		_ctrl.outb(ATA_REG_ADDRESS1,(uint8_t)(lba & 0xFF));
		_ctrl.outb(ATA_REG_ADDRESS2,(uint8_t)(lba >> 8));
		_ctrl.outb(ATA_REG_ADDRESS3,(uint8_t)(lba >> 16));
	}
	else {
		ATA_LOGDETAIL("LBA28: setting sector-count %d and LBA %x",secCount,(uint)(lba & 0xFFFFFFFF));
		/* send sector-count */
		_ctrl.outb(ATA_REG_SECTOR_COUNT,(uint8_t)secCount);
		/* LBA1, LBA2 and LBA3 */
		_ctrl.outb(ATA_REG_ADDRESS1,(uint8_t)lba);
		_ctrl.outb(ATA_REG_ADDRESS2,(uint8_t)(lba >> 8));
		_ctrl.outb(ATA_REG_ADDRESS3,(uint8_t)(lba >> 16));
	}

	/* send command */
	ATA_LOGDETAIL("Sending command %d",cmd);
	_ctrl.outb(ATA_REG_COMMAND,cmd);
	return true;
}
