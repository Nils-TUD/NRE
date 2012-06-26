/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/GlobalEc.h>
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
	bool is_active() const;
	nul::Consumer<nul::Console::SendPacket> &cons() {
		return _cons;
	}
	nul::Producer<nul::Console::ReceivePacket> &prod() {
		return _prod;
	}

	void invalidate();
	void put(const nul::Console::SendPacket &pk);
	void scroll();
	void repaint();

private:
	static void receiver(void *);

private:
	uint _id;
	bool _active;
	nul::GlobalEc _ec;
	nul::Sc _sc;
	nul::DataSpace *_in_ds;
	nul::DataSpace *_out_ds;
	nul::DataSpace _buffer;
	nul::Producer<nul::Console::ReceivePacket> _prod;
	nul::Consumer<nul::Console::SendPacket> _cons;
	ConsoleSessionData *_sess;
	static nul::ForwardCycler<nul::CPU::iterator,nul::LockPolicyDefault<nul::SpinLock> > _cpus;
};
