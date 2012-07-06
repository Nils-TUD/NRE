/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

.extern abort

# called by a return of a global Thread.
.global ec_landing_spot
ec_landing_spot:
	call	abort
	# just to be sure
	1:		jmp 1b
