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
#include <cstdlib>
#include <cstring>

/* Configuration flags for dlmalloc */

#define USE_DL_PREFIX           /* Use dl prefix for all exports */
#define MALLOC_FAILURE_ACTION   /* empty */
#define USE_LOCKS               2
#define HAVE_MORECORE           0
#define HAVE_MMAP               1
#define HAVE_MREMAP             0

#define LACKS_UNISTD_H
#define LACKS_FCNTL_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#define LACKS_SYS_TYPES_H
#define LACKS_ERRNO_H
#define LACKS_SCHED_H
#define LACKS_TIME_H
#define LACKS_STDLIB_H
#define DEFAULT_GRANULARITY     (128 * 1024)        // 128K
#define MALLOC_ALIGNMENT        16                  // important for SSE

/* MMAP dummy */
#define MAP_ANONYMOUS           0
#define MAP_PRIVATE             0
#define PROT_READ               1                   // see DataSpace::R
#define PROT_WRITE              2                   // see DataSpace::W

/* we can't use C++ exceptions here */
#undef assert
#define assert
#define fprintf(f, ...)

EXTERN_C void *mmap(void *start, size_t size, int prot, int flags, int fd, off_t offset);
EXTERN_C int munmap(void *start, size_t size);

typedef struct DlMallocSm {
    long value;
    capsel_t sm;
} DlMallocSm;

EXTERN_C void semaphore_init(DlMallocSm *lk, unsigned initial);
EXTERN_C void semaphore_destroy(DlMallocSm *lk);
EXTERN_C void semaphore_down(DlMallocSm *lk);
EXTERN_C void semaphore_up(DlMallocSm *lk);

#define EINVAL                  -1
#define ENOMEM                  -1

#define DLMALLOC_EXPORT         EXTERN_C

/* EOF */
