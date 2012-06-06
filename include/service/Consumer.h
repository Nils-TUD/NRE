/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

namespace nul {

template<typename T>
class Consumer {
	struct Interface {
		volatile size_t rpos;
		volatile size_t wpos;
		T buffer[];
	};

public:
	Consumer(DataSpace *ds)
		: _ds(ds), _if(reinterpret_cast<Interface*>(ds->virt())), _sm(_ds->sel(),true) {
	}

	bool has_data() const {
		return _if->rpos != _if->wpos;
	}

	T *receive() {
		_sm.down();
	}

	T * get_buffer() {
		return _buffer + _rpos;
	}

	void free_buffer() {
		_rpos = (_rpos + 1) % SIZE;
	}

	Consumer() :
			_rpos(0), _wpos(0) {
	}

private:
	DataSpace *_ds;
	Interface *_if;
	Sm _sm;
};

}
