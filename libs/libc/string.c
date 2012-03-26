/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

typedef unsigned long size_t;

void* memcpy(void *dst,const void *src,size_t count) {
	void *ret = dst;
	asm volatile ("rep movsb" : "+D"(dst), "+S"(src), "+c" (count) : : "memory");
	return ret;
}

void* memmove(void *dst,const void *src,size_t count) {
	char *d = (char*)dst;
	const char *s = (const char*)src;
	if(d > s) {
		d += count - 1;
		s += count - 1;
		asm volatile ("std; rep movsb; cld;" : "+D"(d), "+S"(s), "+c"(count) : : "memory");
	}
	else
		asm volatile ("rep movsb" : "+D"(d), "+S"(s), "+c"(count) : : "memory");
	return dst;
}

void* memset(void *dst,int n,size_t count) {
	void *res = dst;
	asm volatile ("rep stosb" : "+D"(dst), "+a"(n), "+c"(count) : : "memory");
	return res;
}

size_t strnlen(const char *src,size_t maxlen) {
	unsigned long count = maxlen;
	unsigned char ch = 0;
	asm volatile ("repne scasb; setz %0;" : "+a"(ch), "+D"(src), "+c"(count));
	if(ch)
		count--;
	return maxlen - count;
}

size_t strlen(const char *src) {
	return strnlen(src,~0ul);
}

int memcmp(const void *dst,const void *src,size_t count) {
	const char *d = (const char*)dst;
	const char *s = (const char*)src;
	unsigned i;
	for(i = 0; i < count; i++)
		if(s[i] > d[i])
			return 1;
		else if(s[i] < d[i])
			return -1;
	return 0;
}

int strcmp(const char *dst,const char *src) {
	return memcmp(dst,src,strlen(dst));
}
