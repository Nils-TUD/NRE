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
#include <ipc/Connection.h>
#include <ipc/PtClientSession.h>
#include <ipc/Consumer.h>
#include <utcb/UtcbFrame.h>
#include <util/DMA.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

/**
 * Types for the storage service
 */
class Storage {
public:
	typedef ulong tag_type;
	typedef uint64_t sector_type;

	static const size_t MAX_CONTROLLER		= 8;
	static const size_t MAX_DRIVES			= 32;	// per controller
	static const size_t MAX_DMA_DESCS		= 64;

	typedef DMADescList<MAX_DMA_DESCS> dma_type;

	/**
	 * The available commands
	 */
	enum Command {
		INIT,
		READ,
		WRITE,
		FLUSH,
	};

	/**
	 * Describes a drive
	 */
	struct Parameter {
		enum Type {
			FLAG_HARDDISK	= 1,
			FLAG_ATAPI		= 2
		};
		uint flags;
		sector_type sectors;
		size_t sector_size;
		uint max_requests;
		char name[64];
	};

	/**
	 * Completion message
	 */
	struct Packet {
		tag_type tag;
		uint status;

		explicit Packet(tag_type tag,uint status) : tag(tag), status(status) {
		}
	};

private:
	Storage();
};

/**
 * Represents a session at the storage service
 */
class StorageSession : public PtClientSession {
	typedef Storage::tag_type tag_type;
	typedef Storage::sector_type sector_type;

public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 * @param ds the dataspace to use for data exchange
	 * @param drive the drive
	 */
	explicit StorageSession(Connection &con,DataSpace &ds,size_t drive)
			: PtClientSession(con),
			  _ctrlds(ExecEnv::PAGE_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			  _cons(&_ctrlds,true) {
		init(ds,drive);
	}

	/**
	 * @return the consumer to get notified about finished commands
	 */
	Consumer<Storage::Packet> &consumer() {
		return _cons;
	}

	/**
	 * @return the parameters of the drive
	 */
	const Storage::Parameter &get_params() {
		return _params;
	}

	/**
	 * Flushes the disk buffer
	 *
	 * @param tag the tag to identify the command on completion
	 */
	void flush(tag_type tag) {
		UtcbFrame uf;
		uf << Storage::FLUSH << tag;
		pt().call(uf);
		uf.check_reply();
	}

	/**
	 * Reads the sectors <sector>, ..., <sector> + <count> - 1 into the dataspace at given offset
	 *
	 * @param tag the tag to identify the command on completion
	 * @param sector the start sector
	 * @param count the number of sectors
	 * @param offset the offset in the dataspace where to put the data (in bytes)
	 */
	void read(tag_type tag,sector_type sector,sector_type count = 1,size_t offset = 0) {
		Storage::dma_type dma;
		dma.push(DMADesc(offset,count * _params.sector_size));
		read(tag,sector,dma);
	}

	/**
	 * Reads sectors starting at <sector> into the dataspace. Which parts of the read sectors to
	 * put where in the dataspace is described by <dma>. Note that the total bytes in <dma> have
	 * to be a multiple of the sector size.
	 *
	 * @param tag the tag to identify the command on completion
	 * @param sector the start sector
	 * @param dma describes what to transfer where
	 */
	void read(tag_type tag,sector_type sector,const Storage::dma_type &dma) {
		UtcbFrame uf;
		uf << Storage::READ << tag << sector << dma;
		pt().call(uf);
		uf.check_reply();
	}

	/**
	 * Writes the content in the dataspace at offset <offset> to the sectors
	 * <sector>, ..., <sector> + <count> - 1 on disk.
	 *
	 * @param tag the tag to identify the command on completion
	 * @param sector the start sector
	 * @param count the number of sectors
	 * @param offset the offset in the dataspace from where to read the data (in bytes)
	 */
	void write(tag_type tag,sector_type sector,sector_type count = 1,size_t offset = 0) {
		Storage::dma_type dma;
		dma.push(DMADesc(offset,count * _params.sector_size));
		write(tag,sector,dma);
	}

	/**
	 * Writes to sectors starting at <sector> from the dataspace. Which parts of the dataspace to
	 * put where on the drive is described by <dma>. Note that the total bytes in <dma> have
	 * to be a multiple of the sector size.
	 *
	 * @param tag the tag to identify the command on completion
	 * @param sector the start sector
	 * @param dma describes what to transfer where
	 */
	void write(tag_type tag,sector_type sector,const Storage::dma_type &dma) {
		UtcbFrame uf;
		uf << Storage::WRITE << tag << sector << dma;
		pt().call(uf);
		uf.check_reply();
	}

private:
	void init(DataSpace &ds,size_t drive) {
		UtcbFrame uf;
		uf.delegate(_ctrlds.sel(),0);
		uf.delegate(ds.sel(),1);
		uf << Storage::INIT << drive;
		pt().call(uf);
		uf.check_reply();
		uf >> _params;
	}

	DataSpace _ctrlds;
	Consumer<Storage::Packet> _cons;
	Storage::Parameter _params;
};

}
