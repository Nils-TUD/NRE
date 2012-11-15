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

#include <arch/ExecEnv.h>
#include <cstdlib>

using namespace nre;

EXTERN_C void __cxa_finalize(void *d);

void abort() {
    ExecEnv::exit(ExecEnv::EXIT_FAILURE);
}

void thread_exit() {
    ExecEnv::thread_exit();
}

void exit(int code) {
    __cxa_finalize(nullptr);
    ExecEnv::exit(code);
}
