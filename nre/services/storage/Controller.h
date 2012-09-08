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

#include <mem/DataSpace.h>
#include <ipc/Producer.h>
#include <services/Storage.h>

/**
 * The base class for all disk controllers
 */
class Controller {
protected:
	typedef nre::Storage::sector_type sector_type;
	typedef nre::Storage::tag_type tag_type;
	typedef nre::Producer<nre::Storage::Packet> producer_type;

public:
	explicit Controller() {
	}
	virtual ~Controller() {
	}

	/**
	 * Tests whether the given drive exists
	 *
	 * @param drive the drive number (can be anything)
	 * @return true if so
	 */
	virtual bool exists(size_t drive) const = 0;

	/**
	 * @return the number of drives (note: if there are n drives, it does not mean that the drive
	 * 		with numbers 0..n-1 are available)
	 */
	virtual size_t drive_count() const = 0;

	/**
	 * Stores the parameters of the given drive into <params>
	 *
	 * @param drive the drive number (has to be valid)
	 * @param params where to store the parameters
	 */
	virtual void get_params(size_t drive,nre::Storage::Parameter *params) const = 0;

	/**
	 * Flushes the disk cache
	 *
	 * @param drive the drive number (has to be valid)
	 * @param prod the producer to notify when the command is finished
	 * @param tag the tag to use for the notify
	 * @throws Exception if something goes wrong
	 */
	virtual void flush(size_t drive,producer_type *prod,tag_type tag) = 0;

	/**
	 * Reads <bytes> bytes from <offset> in <ds> from sector <sector> of drive <drive>
	 *
	 * @param drive the drive number (has to be valid)
	 * @param prod the producer to notify when the command is finished
	 * @param tag the tag to use for the notify
	 * @param ds the dataspace to read into
	 * @param offset the offset in the dataspace where to store the data
	 * @param sector the start-sector
	 * @param count the number of sectors to read
	 * @throws Exception if something goes wrong
	 */
	virtual void read(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			size_t offset,sector_type sector,sector_type count) = 0;

	/**
	 * Writes <bytes> bytes from sector <sector> of drive <drive> to <offset> in <ds>
	 *
	 * @param drive the drive number (has to be valid)
	 * @param prod the producer to notify when the command is finished
	 * @param tag the tag to use for the notify
	 * @param ds the dataspace to read from
	 * @param sector the start-sector
	 * @param offset the offset in the dataspace from where to read the data
	 * @param bytes the number of bytes to write
	 * @throws Exception if something goes wrong
	 */
	virtual void write(size_t drive,producer_type *prod,tag_type tag,const nre::DataSpace &ds,
			size_t offset,sector_type sector,sector_type count) = 0;
};
