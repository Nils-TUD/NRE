/** @file
 * Helper functions.
 *
 * Copyright (C) 2010, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */
#pragma once

#include <stream/Serial.h>

#define union64(HIGH, LOW)          ({ unsigned long long res; asm ("" : "=A"(res) : "d"(HIGH), "a"(LOW)); res; })

enum {
  INJ_IRQWIN = 0x1000,
  INJ_NMIWIN = 0x0000, // XXX missing
  INJ_WIN    = INJ_IRQWIN | INJ_NMIWIN
};

/**
 * Check whether some address is in a range.
 */
static inline bool in_range(unsigned long address, unsigned long base, unsigned long size) { return (base <= address) && (address <= base + size - 1); }

template<unsigned operand_size>
  static inline void cpu_move(void *tmp_dst, void *tmp_src) {
  // XXX aliasing!
  if (operand_size == 0) *reinterpret_cast<unsigned char *>(tmp_dst) = *reinterpret_cast<unsigned char *>(tmp_src);
  if (operand_size == 1) *reinterpret_cast<unsigned short *>(tmp_dst) = *reinterpret_cast<unsigned short *>(tmp_src);
  if (operand_size == 2) *reinterpret_cast<unsigned int *>(tmp_dst) = *reinterpret_cast<unsigned int *>(tmp_src);
  //MEMORY_BARRIER;
}

/**
 * Transfer bytes from src to dst.
 */
static inline void cpu_move(void * tmp_dst, void *tmp_src, unsigned order) {
  switch (order) {
  case 0:  cpu_move<0>(tmp_dst, tmp_src); break;
  case 1:  cpu_move<1>(tmp_dst, tmp_src); break;
  case 2:  cpu_move<2>(tmp_dst, tmp_src); break;
  default: asm volatile ("ud2a");
  }
}

/**
 * Check whether X is true, output an error and return.
 */
#define check0(X, ...) ({ unsigned __res; if ((__res = (X))) { nul::Serial::get().writef("%s() line %d: '" #X "' error = %x", __func__, __LINE__, __res); nul::Serial::get().writef(" " __VA_ARGS__); nul::Serial::get().writef("\n"); return; }})

/**
 * Check whether X is true, output an error and return RET.
 */
#define check1(RET, X, ...) ({ unsigned __res; if ((__res = (X))) { nul::Serial::get().writef("%s() line %d: '" #X "' error = %x", __func__, __LINE__, __res); nul::Serial::get().writef(" " __VA_ARGS__); nul::Serial::get().writef("\n"); return RET; }})

/**
 * Check whether X is true, output an error and goto RET.
 */
#define check2(GOTO, X, ...) ({ if ((res = (X))) { nul::Serial::get().writef("%s() line %d: '" #X "' error = %x", __func__, __LINE__, res); nul::Serial::get().writef(" " __VA_ARGS__); nul::Serial::get().writef("\n"); goto GOTO; }})

/**
 * Make a dependency on the argument, to avoid that the compiler will touch them.
 */
#define asmlinkage_protect(...) __asm__ __volatile__ ("" : : __VA_ARGS__);
