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

#include <arch/Types.h>
#include <Compiler.h>
#include <Errors.h>

namespace nre {

class SyscallABI {
public:
    static inline void syscall(word_t w0) {
        asm volatile (
            "mov %%esp, %%ecx;"
            "mov $1f, %%edx;"
            "sysenter;"
            "1: ;"
            : "+a" (w0)
            :
            : "ecx", "edx", "memory"
        );
        handle_result(w0);
    }

    static inline void syscall(word_t w0, word_t w1) {
        word_t dummy;
        asm volatile (
            "mov %%esp, %%ecx;"
            "mov $1f, %%edx;"
            "sysenter;"
            "1: ;"
            : "+a" (w0), "=c" (dummy), "=d" (dummy)
            : "D" (w1)
            : "memory"
        );
        handle_result(w0);
    }

    static inline void syscall(word_t w0, word_t w1, word_t w2) {
        word_t dummy;
        asm volatile (
            "mov %%esp, %%ecx;"
            "mov $1f, %%edx;"
            "sysenter;"
            "1: ;"
            : "+a" (w0), "=c" (dummy), "=d" (dummy)
            : "D" (w1), "S" (w2)
            : "memory"
        );
        handle_result(w0);
    }

    static inline void syscall(word_t w0, word_t w1, word_t w2, word_t w3) {
        word_t dummy;
        asm volatile (
            "mov %%esp, %%ecx;"
            "mov $1f, %%edx;"
            "sysenter;"
            "1: ;"
            : "+a" (w0), "=c" (dummy), "=d" (dummy)
            : "D" (w1), "S" (w2), "b" (w3)
            : "memory"
        );
        handle_result(w0);
    }

    static inline void syscall(word_t w0, word_t w1, word_t w2, word_t w3, word_t w4) {
        word_t dummy;
        asm volatile (
            "push %%ebp;"
            "mov %6, %%ebp;"
            "mov %%esp, %%ecx;"
            "mov $1f, %%edx;"
            "sysenter;"
            "1: ;"
            "pop %%ebp;"
            : "+a" (w0), "=c" (dummy), "=d" (dummy)
            : "D" (w1), "S" (w2), "b" (w3), "ir" (w4)
            : "memory"
        );
        handle_result(w0);
    }

    static inline void syscall(word_t w0, word_t w1, word_t w2, word_t w3, word_t w4, word_t &out1,
                               word_t &out2) {
        word_t dummy;
        asm volatile (
            "push %%ebp;"
            "mov %6, %%ebp;"
            "mov %%esp, %%ecx;"
            "mov $1f, %%edx;"
            "sysenter;"
            "1: ;"
            "pop %%ebp;"
            : "+a" (w0), "=c" (dummy), "=d" (dummy), "+D" (w1), "+S" (w2)
            : "b" (w3), "ir" (w4)
            : "memory"
        );
        handle_result(w0);
        out1 = w1;
        out2 = w2;
    }

private:
    SyscallABI();

    static inline void handle_result(uint8_t res) {
        if(EXPECT_FALSE(res != E_SUCCESS))
            throw SyscallException(static_cast<ErrorCode>(res));
    }
};

}
