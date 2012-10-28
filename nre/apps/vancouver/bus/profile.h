/** @file
 * Profiling support.
 *
 * Copyright (C) 2007-2010, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germa
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

#ifdef __x86_64__
#define COUNTER_INC(NAME) \
    ({ \
        word_t dummy = 0; \
        asm volatile( \
            ".section	.data; 1: .string \"" NAME "\";.previous;" \
            ".section	.profile;" \
            ASM_WORD_TYPE " 1b; 2: " ASM_WORD_TYPE " 0,0;.previous;" \
            "movabsq	$2b, %0;" \
            "incq		(%0)" \
            : : "r"(dummy) : "cc" \
        ); \
    })

#define COUNTER_SET(NAME, VALUE) \
    { \
        word_t dummy = 0; \
        asm volatile( \
            ".section	.data; 1: .string \"" NAME "\";.previous;" \
            ".section	.profile;" \
            ASM_WORD_TYPE " 1b; 2: " ASM_WORD_TYPE " 0,0;.previous;" \
            "movabsq	$2b, %1;" \
            "mov		%0,(%1)" \
            : : "r"(static_cast<long>(VALUE)), "r"(dummy) \
        ); \
    }
#else
#define COUNTER_INC(NAME) \
    ({ \
        asm volatile( \
            ".section .data; 1: .string \"" NAME "\";.previous;" \
            ".section	.profile;" \
            ASM_WORD_TYPE " 1b; 2: " ASM_WORD_TYPE " 0,0;.previous;" \
            "incl 2b" \
            : : : "cc" \
        ); \
    })

#define COUNTER_SET(NAME, VALUE) \
    { \
        asm volatile( \
            ".section .data; 1: .string \"" NAME "\";.previous;" \
            ".section	.profile;" \
            ASM_WORD_TYPE " 1b; 2: " ASM_WORD_TYPE " 0,0;.previous;" \
            "mov %0,2b" \
            : : "r"(static_cast<long>(VALUE)) \
        ); \
    }
#endif
