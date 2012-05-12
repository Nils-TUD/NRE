/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <Hip.h>

namespace nul {

size_t Hip::cpu_online_count() const {
	size_t c = 0;
	for(cpu_iterator it = cpu_begin(); it != cpu_end(); ++it) {
		if(it->enabled())
			c++;
	}
	return c;
}

}
