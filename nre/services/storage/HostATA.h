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

#include <arch/Types.h>
#include <stream/Serial.h>
#include <cstring>

/**
 * Helper class that unifies functions for IDE+SATA disks
 */
class HostATA {
public:
	explicit HostATA() : _lba48(false), _maxsector(0), _maxcount(0), _atapi(false) {
	}

	/**
	 * Update the parameters from an identify_drive packet.
	 */
	unsigned update_params(uint16_t *id,bool slave) {
		_slave = slave;
		uint8_t checksum = 0;
		for(size_t i = 0; i < 512; i++)
			checksum += reinterpret_cast<uint8_t*>(id)[i];
		if((id[255] & 0xff) == 0xa5 && checksum)
			return 0x11;
		if(((id[0] >> 14) & 3) == 3)
			return 0x12;
		_atapi = id[0] >> 15;
		_maxsector = 0;

		if(!_atapi) {
			_lba48 = id[86] & (1 << 10);
			if(~id[49] & (1 << 9))
				// we need to have LBA and do not support the very old CHS mode anymore!
				_maxsector = 0;
			else
				_maxsector = _lba48 ? *reinterpret_cast<uint64_t*>(id + 100)
						: *reinterpret_cast<uint32_t*>(id + 60);
			_maxcount = (1 << (_lba48 ? 16 : 8)) - 1;
		}
		ntos_string(_model,reinterpret_cast<char*>(id + 27),40);
		for(size_t i = 40; i > 0 && _model[i - 1] == ' '; i--)
			_model[i - 1] = 0;
		_exists = true;
		return 0;
	}

	/**
	 * Get the disk parameters.
	 */
	unsigned get_disk_parameter(nre::Storage::Parameter *params) const {
		params->flags = _atapi ? nre::Storage::Parameter::FLAG_ATAPI :
						(_maxsector ? nre::Storage::Parameter::FLAG_HARDDISK : 0);
		params->sectors = _maxsector;
		params->sector_size = _atapi ? 2048 : 512;
		params->max_requests = _maxcount;
		memcpy(params->name,_model,40);
		return 0;
	}

private:
	static void ntos_string(char *dst,char *str,size_t len) {
		for(size_t i = 0; i < len / 2; i++) {
			char value = str[i * 2];
			dst[i * 2] = str[i * 2 + 1];
			dst[i * 2 + 1] = value;
		}
	}

public:
	bool _exists;
	bool _lba48;
	uint64_t _maxsector;
	uint _maxcount;
	char _model[40];
	bool _atapi;
	bool _slave;
};
