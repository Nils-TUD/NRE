/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>
#include <service/Consumer.h>
#include <service/Producer.h>
#include <dev/Console.h>
#include <util/ScopedLock.h>
#include <util/Cycler.h>
#include <util/DList.h>
#include <CPU.h>

class ConsoleSessionData;

class ConsoleSessionView : public nul::RCUObject, public nul::DListItem {
public:
	ConsoleSessionView(ConsoleSessionData *sess,uint id,nul::DataSpace *in_ds,nul::DataSpace *out_ds);
	virtual ~ConsoleSessionView();

	uint id() const {
		return _id;
	}
	uint uid() const {
		return _uid;
	}
	ConsoleSessionData *session() {
		return _sess;
	}

	void swap();
	nul::Producer<nul::Console::ReceivePacket> &prod() {
		return _prod;
	}
	nul::DataSpace &out_ds() {
		return *_out_ds;
	}

private:
	uint _id;
	uint _uid;
	uint _pos;
	bool _active;
	nul::DataSpace *_in_ds;
	nul::DataSpace *_out_ds;
	nul::Producer<nul::Console::ReceivePacket> _prod;
	ConsoleSessionData *_sess;
	static nul::ForwardCycler<nul::CPU::iterator,nul::LockPolicyDefault<nul::SpinLock> > _cpus;
	static uint _next_uid;
};
