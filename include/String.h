/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
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
