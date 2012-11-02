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
#include <arch/ExecEnv.h>
#include <stream/OStream.h>
#include <stream/OStringStream.h>
#include <Exception.h>

namespace nre {

class Util {
public:
    static void panic(const char *fmt, ...) {
        static char msg[256];
        va_list ap;
        OStringStream os(msg, sizeof(msg));
        va_start(ap, fmt);
        os.vwritef(fmt, ap);
        va_end(ap);
        throw Exception(E_FAILURE, msg);
    }

    template<typename T>
    static void swap(T &t1, T &t2) {
        T tmp = t1;
        t1 = t2;
        t2 = tmp;
    }

    static void write_dump(OStream& os, void *p, size_t size) {
        word_t *ws = reinterpret_cast<word_t*>(p);
        for(size_t i = 0; i < size / sizeof(word_t); ++i)
            os.writef("%p: %#0" FMT_WORD_HEXLEN "lx\n", ws + i, ws[i]);
    }

    static void write_backtrace(OStream& os) {
        uintptr_t addrs[32];
        ExecEnv::collect_backtrace(addrs, sizeof(addrs));
        write_backtrace(os, addrs);
    }
    static void write_backtrace(OStream& os, const uintptr_t *addrs) {
        const uintptr_t *addr;
        os.writef("Backtrace:\n");
        addr = addrs;
        while(*addr != 0) {
            os.writef("\t%p\n", reinterpret_cast<void*>(*addr));
            addr++;
        }
    }

    // TODO move that to somewhere else
    static uint32_t cpuid(uint32_t eax, uint32_t &ebx, uint32_t &ecx, uint32_t &edx) {
        asm volatile ("cpuid" : "+a" (eax), "+b" (ebx), "+c" (ecx), "+d" (edx));
        return eax;
    }
    static void pause() {
        asm volatile ("pause");
    }
    static timevalue_t tsc() {
        uint32_t u, l;
        asm volatile ("rdtsc" : "=a" (l), "=d" (u));
        return (timevalue_t)u << 32 | l;
    }

private:
    Util();
};

}
