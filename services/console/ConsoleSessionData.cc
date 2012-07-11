/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <RCU.h>
#include <cstring>

#include "ConsoleSessionData.h"

using namespace nre;

void ConsoleSessionData::create(DataSpace *in_ds,DataSpace *out_ds,bool show_pages) {
	ScopedLock<UserSm> guard(&_sm);
	if(_in_ds != 0)
		throw Exception(E_EXISTS);
	_in_ds = in_ds;
	_out_ds = out_ds;
	if(_in_ds)
		_prod = new Producer<Console::ReceivePacket>(in_ds,false,false);
	_show_pages = show_pages;
	ConsoleService::get()->switcher().switch_to(ConsoleService::get()->active(),this);
}

void ConsoleSessionData::portal(capsel_t pid) {
	UtcbFrameRef uf;
	try {
		ScopedLock<RCULock> guard(&RCU::lock());
		ConsoleSessionData *sess = ConsoleService::get()->get_session<ConsoleSessionData>(pid);
		Console::Command cmd;
		uf >> cmd;
		switch(cmd) {
			case Console::CREATE: {
				bool show_pages;
				DataSpaceDesc indesc,outdesc;
				capsel_t insel = uf.get_delegated(0).offset();
				capsel_t outsel = uf.get_delegated(0).offset();
				uf >> indesc >> outdesc >> show_pages;
				uf.finish_input();

				sess->create(new DataSpace(indesc,insel),new DataSpace(outdesc,outsel),show_pages);
				uf.accept_delegates();
				uf << E_SUCCESS;
			}
			break;

			case Console::SET_REGS: {
				Console::Register regs;
				uf >> regs;
				uf.finish_input();

				sess->set_regs(regs);
				uf << E_SUCCESS;
			}
			break;
		}
	}
	catch(const Exception &e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}
