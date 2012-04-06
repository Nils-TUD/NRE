/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <Types.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void *dest,const void *src,size_t len);
void *memmove(void *dest,const void *src,size_t count);
void *memset(void *addr,int value,size_t count);
size_t strlen(const char *src);
int memcmp(const void *str1,const void *str2,size_t count);
int strcmp(const char *dst,const char *src);

#ifdef __cplusplus
}
#endif
