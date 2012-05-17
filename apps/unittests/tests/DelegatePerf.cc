/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <cap/Caps.h>
#include <Util.h>
#include <CPU.h>
#include "DelegatePerf.h"

using namespace nul;
using namespace nul::test;

PORTAL static void portal_test(cap_t);
static void test_delegate();

const TestCase delegateperf = {
	"Delegate-performance",test_delegate
};

static const unsigned tries = 10000;
static uint64_t results[tries];

static void portal_test(cap_t) {
	UtcbFrameRef uf;
	uf.add_typed(DelItem(Crd(0x100,2,DESC_IO_ALL),0,0));
}

static void test_delegate() {
	Caps::allocate(CapRange(0x100,1 << 2,Caps::TYPE_IO | Caps::IO_A));

	LocalEc ec(CPU::current().id);
	Pt pt(&ec,portal_test);
	uint64_t tic,tac,min = ~0ull,max = 0,ipc_duration,rdtsc;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;
	UtcbFrame uf;
	for(unsigned i = 0; i < tries; i++) {
		tic = Util::tsc();
		uf.reset();
		uf.set_receive_crd(Crd(0,31,DESC_IO_ALL));
		pt.call(uf);
		tac = Util::tsc();
		ipc_duration = tac - tic - rdtsc;
		min = Util::min(min,ipc_duration);
		max = Util::max(max,ipc_duration);
		results[i] = ipc_duration;
	}
	uint64_t avg = 0;
	for(unsigned i = 0; i < tries; i++)
		avg += results[i];

	avg = avg / tries;
	WVPERF(avg,"cycles");
	Log::get().writef("avg: %Lu\n",avg);
	Log::get().writef("min: %Lu\n",min);
	Log::get().writef("max: %Lu\n",max);
}
