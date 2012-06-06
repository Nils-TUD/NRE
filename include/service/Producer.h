/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <mem/DataSpace.h>
#include <service/Consumer.h>
#include <Util.h>

namespace nul {

template<typename T>
class Producer {
public:
	Producer(DataSpace *ds)
		: _ds(ds), _if(reinterpret_cast<typename Consumer<T>::Interface*>(ds->virt())),
		  _max((ds->size() - sizeof(typename Consumer<T>::Interface)) / sizeof(T)),
		  _sm(_ds->sel(),true) {
		_if->rpos = 0;
		_if->wpos = 0;
	}

	bool produce(T &value) {
		if((_if->wpos + 1) % _max == _if->rpos)
			return false;
		_if->buffer[_if->wpos] = value;
		_if->wpos = (_if->wpos + 1) % _max;
		Util::memory_barrier();
		_sm.up();
		return true;
	}

private:
	DataSpace *_ds;
	typename Consumer<T>::Interface *_if;
	size_t _max;
	Sm _sm;
};

}
