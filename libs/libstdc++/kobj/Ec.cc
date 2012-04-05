/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Ec.h>

uintptr_t Ec::_utcb_addr = 0x1000000;

void *ec_stacks[Ec::MAX_STACKS][Ec::STACK_SIZE / sizeof(void*)] __attribute__((aligned(Ec::STACK_SIZE)));
size_t ec_stack = 0;
