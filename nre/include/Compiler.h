/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#define REGPARMS(X)             __attribute__ ((regparm(X)))
#define ALIGNED(X)              __attribute__ ((aligned(X)))
#define PACKED                  __attribute__ ((packed))
#define NORETURN                __attribute__ ((__noreturn__))
#define NOINLINE                __attribute__ ((noinline))
#define INIT_PRIORITY(X)        __attribute__ ((init_priority((X))))
#define WEAK                    __attribute__ ((weak))
#define FMT_PRINTF(X, Y)        __attribute__ ((format (printf, (X), (Y))))
#define EXPECT_FALSE(X)         __builtin_expect(!!(X), 0)
#define EXPECT_TRUE(X)          __builtin_expect(!!(X), 1)
#define UNUSED                  __attribute__ ((unused))
#define UNREACHED               __builtin_unreachable()

#ifdef __clang__
#    define COMPILER_NAME       "clang " __VERSION__
#else
#    define COMPILER_NAME       "gcc " __VERSION__
#endif

#ifdef __cplusplus
#    define EXTERN_C             extern "C"
#else
#    define EXTERN_C
#endif
