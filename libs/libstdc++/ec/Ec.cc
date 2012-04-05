/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <ec/Ec.h>

void *Ec::_stacks[MAX_STACKS][STACK_SIZE / sizeof(void*)];
size_t Ec::_stack = 0;
uintptr_t Ec::_utcb_addr = 0x1000000;
