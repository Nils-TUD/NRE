/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2008-2009, Bernhard Kauer <bk@vmmon.org>
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
#include <util/Clock.h>
#include <util/PCI.h>
#include <Logging.h>

#include "HostATA.h"
#include "Controller.h"

/**
 * A simple IDE host driver.
 *
 * State: testing
 * Features: lba28, lba48, PIO
 * Missing: dma, IRQ
 */
class HostIDE : public Controller {
	static const uint FREQ = 1000; // millisecond

public:
	explicit HostIDE(nre::Ports::port_t iobase)
			: Controller(), _sm(), _params(), _clock(FREQ), _ports(iobase,8), _drive_count(0) {
		unsigned short buffer[256];
		if(!identify_drive(buffer,_params[0],false))
			_drive_count++;
		if(!identify_drive(buffer,_params[1],true))
			_drive_count++;
	}

	virtual size_t drive_count() const {
		return _drive_count;
	}

	virtual bool exists(size_t drive) const {
		return drive < 2 && _params[drive]._exists;
	}

	virtual void get_params(size_t drive,nre::Storage::Parameter *params) const {
		_params[drive].get_disk_parameter(params);
	}

	virtual void flush(size_t drive,producer_type *prod,tag_type tag) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		uint8_t packets[18];
		HostATA &params = _params[drive];
		// XXX handle RO media
		if(!make_sector_packets(params,packets,params._lba48 ? 0xea : 0xe7,0,0))
			throw nre::Exception(nre::E_FAILURE,"Creating packet failed");
		uint res;
		if((res = ata_command(packets + 8,0,0,true)))
			throw nre::Exception(nre::E_FAILURE,32,"ATA command failed with %#x",res);
		prod->produce(nre::Storage::Packet(tag,0));
	}

	virtual void read(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		readwrite(drive,prod,tag,ds,sector,offset,bytes,false);
	}

	virtual void write(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		readwrite(drive,prod,tag,ds,sector,offset,bytes,true);
	}

private:
	void readwrite(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			sector_type sector,size_t offset,size_t bytes,bool write) {
		HostATA &params = _params[drive];
		if(bytes & 0x1FF)
			throw nre::Exception(nre::E_ARGS_INVALID,"Can only transfer complete sectors");

		uint8_t command = (params._lba48 ? 0x24 : 0x20);
		if(write)
			command += 0x10;
		uint8_t packets[18];
		size_t pos = 0;
		uint res = 0;
		while(pos < bytes) {
			// build sector packet
			if(!make_sector_packets(params,packets,command,sector,1))
				throw nre::Exception(nre::E_FAILURE,"Unable to build packet");
			if(params._lba48)
				send_packet(packets);

			void *buffer = reinterpret_cast<void*>(ds.virt() + offset);
			if((res = ata_command(packets + 8,buffer,512,!write)))
				throw nre::Exception(nre::E_FAILURE,32,"ATA command failed with %#x",res);

			pos += 512;
			sector++;
		}
		prod->produce(nre::Storage::Packet(tag,0));
	}

	/**
	 * Wait with timeout for the right disk state.
	 */
	uint wait_disk_state(uint8_t mask,uint8_t value,timevalue_t msec,bool check_error) {
		uint8_t status;
		timevalue_t timeout = _clock.source_time(msec);
		do {
			status = _ports.in<uint8_t>(7);
			if(check_error && (status & 0x21))
				return (_ports.in<uint8_t>(1) << 8) | status;
			nre::Util::pause();
		}
		while((status & mask) != value && _clock.source_time() < timeout);
		return (status & mask) != value;
	}

	uint send_packet(const uint8_t *packet) {
		// wait 10ms that BSY clears
		uint res = wait_disk_state(0x80,0,10,false);
		if(res)
			return res;

		size_t index = 0;
		for(uint i = 0; i < 8; i++) {
			if(packet[0] & (1 << i))
				_ports.out<uint8_t>(packet[++index],i);
		}
		return 0;
	}

	/**
	 * Send a packet to the ata controller.
	 */
	uint ata_command(uint8_t *packet,void *buffer,size_t outlen,bool read) {
		uint res = send_packet(packet);
		if(!res && outlen) {
			uint16_t *words = reinterpret_cast<uint16_t*>(buffer);
			outlen /= 2;
			if(read) {
				// wait 10seconds that we got the data
				if(wait_disk_state(0x88,0x08,10000,true))
					return ~0x3ffU | _ports.in<uint8_t>(7);
				for(size_t i = 0; i < outlen; ++i)
					words[i] = _ports.in<uint16_t>();
			}
			else {
				// wait 100ms that we can send the data
				if(wait_disk_state(0x88,0x08,100,true))
					return ~0x4ffU | _ports.in<uint8_t>(7);
				for(size_t i = 0; i < outlen; ++i)
					_ports.out<uint16_t>(words[i]);
			}
			// wait 20ms to let status settle and make sure there is no data left to transfer
			if(wait_disk_state(0x88,0,20,true))
				return ~0x5ffU | _ports.in<uint8_t>(7);
		}
		return res;
	}

	/**
	 * Initialize packets for a single sector LBA function.
	 */
	bool make_sector_packets(HostATA &params,uint8_t *packets,uint8_t command,
			sector_type sector,size_t count) {
		if(sector >= params._maxsector || sector > (params._maxsector - count))
			return false;
		if((count >> 16) || (!params._lba48 && count >> 8))
			return false;
		packets[0] = 0x3c;
		packets[1] = count >> 8;
		packets[2] = sector >> 24;
		packets[3] = sector >> 32;
		packets[4] = sector >> 40;
		packets[8] = 0xfc;
		packets[9] = count;
		packets[10] = sector >> 0;
		packets[11] = sector >> 8;
		packets[12] = sector >> 16;
		packets[13] = (params._slave ? 0xf0 : 0xe0) | ((sector >> 24) & (params._lba48 ? 0 : 0xf));
		packets[14] = command;
		return true;
	}

	uint identify_drive(uint16_t *buffer,HostATA &params,bool slave) {
		// select drive
		uint8_t packet_select[] = {0x40,static_cast<uint8_t>(slave ? 0xb0 : 0xa0)};
		send_packet(packet_select);
		// select used and device ready
		if((_ports.in<uint8_t>(6) & 0x10) != (slave ? 0x10 : 0) || (~_ports.in<uint8_t>(7) & 0x40))
			return -1;

		uint8_t packet[] = {0x80,0xec};
		uint res = ata_command(packet,buffer,512,true);

		// abort?
		if(((res & 0x401) == 0x401) && ((res & 0x7f) != 0x7f)) {
			if(_ports.in<uint8_t>(4) == 0x14 && _ports.in<uint8_t>(5) == 0xeb) {
				uint8_t atapi_packet[] = {0xc0,static_cast<uint8_t>(slave ? 0xb0 : 0xa0),0xa1};
				res = ata_command(atapi_packet,buffer,512,true);
			}
		}
		if(!res)
			res = params.update_params(buffer,slave);
		else {
			LOG(nre::Logging::STORAGE,
					nre::Serial::get().writef("ATA: identify packet err %#x\n",res));
		}

		if(!res) {
			nre::Storage::Parameter info;
			params.get_disk_parameter(&info);
			LOG(nre::Logging::STORAGE,
					nre::Serial::get().writef("%s drive #%zu present (%s)\n"
							"  %Lu sectors with of %zu bytes, max %zu requests\n",
							params._atapi ? "ATAPI" : "ATA",&params - _params,
							info.name,info.sectors,info.sector_size,info.max_requests));
		}
		return res;
	}

	nre::UserSm _sm;
	HostATA _params[2];
	nre::Clock _clock;
	nre::Ports _ports;
	size_t _drive_count;
};
