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

PORTAL static void portal_test(capsel_t);
static void test_delegate();

const TestCase delegateperf = {
	"Delegate-performance",test_delegate
};

static const size_t tries = 1000;
static uint64_t results[tries];

static void portal_test(capsel_t) {
	UtcbFrameRef uf;
	uf.delegate(CapRange(0x100,4,Crd::IO_ALL));
}

static void test_delegate() {
	Caps::allocate(CapRange(0x100,1 << 2,Crd::IO_ALL));

	LocalEc ec(CPU::current().id);
	Pt pt(&ec,portal_test);
	uint64_t tic,tac,min = ~0ull,max = 0,ipc_duration,rdtsc;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;
	UtcbFrame uf;
	uf.set_receive_crd(Crd(0,31,Crd::IO_ALL));
	for(size_t i = 0; i < tries; i++) {
		tic = Util::tsc();
		pt.call(uf);
		uf.clear();
		tac = Util::tsc();
		ipc_duration = tac - tic - rdtsc;
		min = Math::min(min,ipc_duration);
		max = Math::max(max,ipc_duration);
		results[i] = ipc_duration;
	}
	uint64_t avg = 0;
	for(size_t i = 0; i < tries; i++)
		avg += results[i];

	avg = avg / tries;
	WVPERF(avg,"cycles");
	WVPRINTF("avg: %Lu\n",avg);
	WVPRINTF("min: %Lu\n",min);
	WVPRINTF("max: %Lu\n",max);
}
