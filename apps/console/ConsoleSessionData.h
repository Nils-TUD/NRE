/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <service/Service.h>
#include <service/Consumer.h>
#include <service/Producer.h>
#include <mem/DataSpace.h>
#include <dev/Console.h>
#include <dev/Screen.h>
#include <CPU.h>

#include "ConsoleService.h"

class ConsoleSessionData : public nul::SessionData {
public:
	ConsoleSessionData(nul::Service *s,size_t id,capsel_t caps,nul::Pt::portal_func func);
	virtual ~ConsoleSessionData();

	virtual void invalidate();

	void put(const nul::Console::SendPacket &pk);
	void scroll(uint view);
	void switch_to(uint view);

	uint view() const {
		return _view;
	}
	nul::UserSm &sm() {
		return _sm;
	}
	nul::Consumer<nul::Console::SendPacket> *cons() {
		return _cons;
	}
	nul::Producer<nul::Console::ReceivePacket> *prod() {
		return _prod;
	}

protected:
	virtual void accept_ds(nul::DataSpace *ds);

private:
	static cpu_t next_cpu() {
		static nul::UserSm sm;
		nul::ScopedLock<nul::UserSm> guard(&sm);
		if(_cpu_it == nul::CPU::end())
			_cpu_it = nul::CPU::begin();
		return (_cpu_it++)->id;
	}

	static void receiver(void *);

	uint _view;
	nul::UserSm _sm;
	nul::GlobalEc _ec;
	nul::Sc *_sc;
	nul::DataSpace *_in_ds;
	nul::DataSpace *_out_ds;
	nul::DataSpace _buffer;
	nul::Producer<nul::Console::ReceivePacket> *_prod;
	nul::Consumer<nul::Console::SendPacket> *_cons;
	static nul::CPU::iterator _cpu_it;
};
