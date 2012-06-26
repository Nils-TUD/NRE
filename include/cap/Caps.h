/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <cap/CapRange.h>
#include <utcb/UtcbFrame.h>
#include <CPU.h>
#include <util/Math.h>

namespace nul {

// TODO what to do with this class?
class Caps {
public:
	static void allocate(const CapRange& caps) {
		UtcbFrame uf;
		uf.set_receive_crd(Crd(0,31,caps.attr()));
		CapRange cr = caps;
		size_t count = cr.count();
		while(count > 0) {
			uf.clear();
			if(cr.hotspot()) {
				// if start and hotspot are different, we have to check whether it fits in the utcb
				uintptr_t diff = cr.hotspot() ^ cr.start();
				if(diff) {
					// the lowest bit that's different defines how many we can map with one Crd.
					// with bit 0, its 2^0 = 1 at once, with bit 1, 2^1 = 2 and so on.
					unsigned at_once = Math::bit_scan_forward(diff);
					if((1 << at_once) < count) {
						// be carefull that we might have to align it to 1 << at_once first. this takes
						// at most at_once typed items.
						size_t free = uf.free_typed() - (sizeof(CapRange) / (sizeof(word_t) * 2));
						size_t min = Math::min<uintptr_t>(free - at_once,count >> at_once);
						cr.count(min << at_once);
					}
				}
			}

			uf << cr;
			CPU::current().io_pt->call(uf);

			// adjust start and hotspot for the next round
			cr.start(cr.start() + cr.count());
			if(cr.hotspot())
				cr.hotspot(cr.hotspot() + cr.count());
			count -= cr.count();
			cr.count(count);
		}
	}

	static void free(const CapRange&) {
		// TODO
	}

private:
	Caps();
};

}
