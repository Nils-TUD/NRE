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
		{COMMAND_READ_SEC, COMMAND_READ_SEC_EXT},
		{COMMAND_WRITE_SEC, COMMAND_WRITE_SEC_EXT},
		{COMMAND_READ_DMA, COMMAND_READ_DMA_EXT},
		{COMMAND_WRITE_DMA, COMMAND_WRITE_DMA_EXT}
	};
	uint offset;
	if(op == PACKET)
		return COMMAND_PACKET;
	offset = (_ctrl.dma_enabled() && has_dma()) ? 2 : 0;
	if(op == WRITE)
		offset++;
	return commands[offset][has_lba48() ? 1 : 0];
}

void HostATADevice::readwrite(Operation op, const DataSpace &ds, sector_type sector,
                              const dma_type &dma, producer_type *prod, tag_type tag, size_t secsize) {
	if(secsize == 0)
		secsize = _sector_size;
	uint cmd = get_command(op);
	setup_command(sector, dma.bytecount() / secsize, cmd);

	switch(cmd) {
		case COMMAND_PACKET:
		case COMMAND_READ_SEC:
		case COMMAND_READ_SEC_EXT:
		case COMMAND_WRITE_SEC:
		case COMMAND_WRITE_SEC_EXT:
			transferPIO(op, ds, secsize, dma, prod, tag, true);
			break;
		case COMMAND_READ_DMA:
		case COMMAND_READ_DMA_EXT:
		case COMMAND_WRITE_DMA:
		case COMMAND_WRITE_DMA_EXT:
			transferDMA(op, ds, dma, prod, tag);
			break;
		default:
			throw Exception(E_ARGS_INVALID, "Invalid command");
	}
}

void HostATADevice::flush_cache() {
	// wait until the drive is ready
	int res = _ctrl.wait_until(PIO_TRANSFER_TIMEOUT, CMD_ST_READY, 0);
	_ctrl.handle_status(_id, res, "Flush cache");

	// select drive
	_ctrl.outb(ATA_REG_DRIVE_SELECT, (_id & SLAVE_BIT) << 4);

	// send command
	_ctrl.outb(ATA_REG_COMMAND, has_lba48() ? COMMAND_FLUSH_CACHE_EXT : COMMAND_FLUSH_CACHE);

	// wait until BSY and DRQ cleared; RDY should be set
	res = _ctrl.wait_until(PIO_TRANSFER_TIMEOUT, CMD_ST_READY, CMD_ST_BUSY | CMD_ST_DRQ);
	_ctrl.handle_status(_id, res, "Flush cache");
}

void HostATADevice::transferPIO(Operation op, const DataSpace &ds, size_t secsize,
                                const dma_type &dma, producer_type *prod, tag_type tag,
                                bool waitfirst) {
	size_t offset = 0;
	size_t length = dma.bytecount();
	int res;
	while(length > offset) {
		if(op == WRITE && dma.in(buffer(), secsize, offset, ds))
			throw Exception(E_ARGS_INVALID, 64, "Device %u: Unable to copyin data", _id);
		if(offset > 0 || waitfirst) {
			if(op == READ)
				_ctrl.wait_ready();
			res = _ctrl.wait_until(PIO_TRANSFER_TIMEOUT, CMD_ST_DRQ, CMD_ST_BUSY);
			_ctrl.handle_status(_id, res, "PIO transfer");
		}

		// now read / write the data
		_ctrl.start_transfer(0, 0, false);
		if(op == READ)
			_ctrl.inwords(ATA_REG_DATA, reinterpret_cast<uint16_t*>(buffer()), secsize / sizeof(uint16_t));
		else
			_ctrl.outwords(ATA_REG_DATA, reinterpret_cast<uint16_t*>(buffer()), secsize /
			               sizeof(uint16_t));

		if(op == READ && dma.out(buffer(), secsize, offset, ds))
			throw Exception(E_ARGS_INVALID, 64, "Device %u: Unable to copyout data", _id);
		offset += secsize;
	}
	if(prod)
		prod->produce(Storage::Packet(tag, 0));
}

void HostATADevice::transferDMA(Operation op, const DataSpace &ds, const dma_type &dma,
                                producer_type *prod, tag_type tag) {
	// wait until previous transfers are done
	ATA_LOGDETAIL("Waiting for previous transfers");
	_ctrl.wait_ready();

	// setup PRDTs
	ATA_LOGDETAIL("Setting PRDs");
	HostIDECtrl::PRD *prd = _ctrl.prdt();
	for(dma_type::iterator it = dma.begin(); it != dma.end(); ) {
		if(((static_cast<uint64_t>(ds.phys()) + it->offset + it->count) >= 0x100000000))
			throw Exception(E_ARGS_INVALID, 64, "Physical address %p is too large for DMA", ds.phys());

		if(it->offset > ds.size() || it->offset + it->count > ds.size()) {
			throw Exception(E_ARGS_INVALID, 64, "Device %u: Invalid offset(%zu)/count(%zu)",
			                it->offset, it->count);
		}
		prd->buffer = static_cast<uint32_t>(ds.phys() + it->offset);
		prd->byteCount = it->count;
		prd->last = ++it == dma.end();
		prd++;
	}

	// stop running transfers
	//ATA_LOGDETAIL("Stopping running transfers");
	//_ctrl.outbmrb(BMR_REG_COMMAND,0);
	uint8_t status = _ctrl.inbmrb(BMR_REG_STATUS) | BMR_STATUS_ERROR | BMR_STATUS_IRQ;
	_ctrl.outbmrb(BMR_REG_STATUS, status);

	// set PRDT
	ATA_LOGDETAIL("Setting PRDT");
	_ctrl.outbmrl(BMR_REG_PRDT, static_cast<uint32_t>(_ctrl.prdt_addr()));

	// it seems to be necessary to read those ports here
	ATA_LOGDETAIL("Starting DMA-transfer");
	_ctrl.inbmrb(BMR_REG_COMMAND);
	_ctrl.inbmrb(BMR_REG_STATUS);
	_ctrl.start_transfer(prod, tag, true);
	// start bus-mastering
	if(op == READ)
		_ctrl.outbmrb(BMR_REG_COMMAND, BMR_CMD_START | BMR_CMD_READ);
	else
		_ctrl.outbmrb(BMR_REG_COMMAND, BMR_CMD_START);
	_ctrl.inbmrb(BMR_REG_COMMAND);
	_ctrl.inbmrb(BMR_REG_STATUS);

	// now wait for an interrupt
	//ATA_LOGDETAIL("Waiting for an interrupt");
	//_ctrl.wait_irq();

	//int res = _ctrl.wait_until(DMA_TRANSFER_TIMEOUT,0,CMD_ST_BUSY | CMD_ST_DRQ);
	//_ctrl.handle_status(_id,res,"DMA transfer");

	//_ctrl.inbmrb(BMR_REG_STATUS);
	//_ctrl.outbmrb(BMR_REG_COMMAND,0);
	//ATA_LOGDETAIL("DMA transfer done");
}

void HostATADevice::setup_command(sector_type sector, sector_type count, uint cmd) {
	uint8_t devValue;
	if(!has_lba48()) {
		if(sector & 0xFFFFFFFFF0000000LL)
			throw Exception(E_ARGS_INVALID, 64, "Trying to read from %#Lx with LBA28", sector);
		if(count & 0xFF00)
			throw Exception(E_ARGS_INVALID, 64, "Trying to read %Lu sectors with LBA28", count);

		// For LBA28, the lowest 4 bits are bits 27-24 of LBA
		devValue = DEVICE_LBA | ((_id & SLAVE_BIT) << 4) | ((sector >> 24) & 0x0F);
		_ctrl.outb(ATA_REG_DRIVE_SELECT, devValue);
	}
	else {
		devValue = DEVICE_LBA | ((_id & SLAVE_BIT) << 4);
		_ctrl.outb(ATA_REG_DRIVE_SELECT, devValue);
	}

	ATA_LOGDETAIL("Selecting device %d (%s)", _id, is_atapi() ? "ATAPI" : "ATA");
	_ctrl.wait();

	ATA_LOGDETAIL("Resetting control-register");
	// reset control-register
	_ctrl.ctrloutb(_ctrl.irqs_enabled() ? 0 : CTRL_NIEN);

	// needed for ATAPI
	if(is_atapi())
		_ctrl.outb(ATA_REG_FEATURES, _ctrl.dma_enabled() && has_dma());

	if(has_lba48()) {
		ATA_LOGDETAIL("LBA48: setting sector-count %Lu and LBA %Lu", count, sector);
		// LBA: | LBA6 | LBA5 | LBA4 | LBA3 | LBA2 | LBA1 |
		//     48             32            16            0
		// sector-count high-byte
		_ctrl.outb(ATA_REG_SECTOR_COUNT, (uint8_t)(count >> 8));
		// LBA4, LBA5 and LBA6
		_ctrl.outb(ATA_REG_ADDRESS1, (uint8_t)(sector >> 24));
		_ctrl.outb(ATA_REG_ADDRESS2, (uint8_t)(sector >> 32));
		_ctrl.outb(ATA_REG_ADDRESS3, (uint8_t)(sector >> 40));
		// sector-count low-byte
		_ctrl.outb(ATA_REG_SECTOR_COUNT, (uint8_t)(count & 0xFF));
		// LBA1, LBA2 and LBA3
		_ctrl.outb(ATA_REG_ADDRESS1, (uint8_t)(sector & 0xFF));
		_ctrl.outb(ATA_REG_ADDRESS2, (uint8_t)(sector >> 8));
		_ctrl.outb(ATA_REG_ADDRESS3, (uint8_t)(sector >> 16));
	}
	else {
		ATA_LOGDETAIL("LBA28: setting sector-count %Lu and LBA %Lu", count, sector);
		// send sector-count
		_ctrl.outb(ATA_REG_SECTOR_COUNT, (uint8_t)count);
		// LBA1, LBA2 and LBA3
		_ctrl.outb(ATA_REG_ADDRESS1, (uint8_t)sector);
		_ctrl.outb(ATA_REG_ADDRESS2, (uint8_t)(sector >> 8));
		_ctrl.outb(ATA_REG_ADDRESS3, (uint8_t)(sector >> 16));
	}

	// send command
	ATA_LOGDETAIL("Sending command %d", cmd);
	_ctrl.outb(ATA_REG_COMMAND, cmd);
}
