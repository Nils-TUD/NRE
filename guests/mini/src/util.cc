#include "util.h"

void Util::move(void *dest,const void *src,size_t count) {
	uchar *s,*d;
	/* nothing to do? */
	if((uchar*)dest == (uchar*)src || count == 0)
		return;

	/* moving forward */
	if((uintptr_t)dest > (uintptr_t)src) {
		uint *dsrc = (uint*)((uintptr_t)src + count - sizeof(uint));
		uint *ddest = (uint*)((uintptr_t)dest + count - sizeof(uint));
		while(count >= sizeof(uint)) {
			*ddest-- = *dsrc--;
			count -= sizeof(uint);
		}
		s = (uchar*)dsrc + (sizeof(uint) - 1);
		d = (uchar*)ddest + (sizeof(uint) - 1);
		while(count-- > 0)
			*d-- = *s--;
	}
	/* moving backwards */
	else
		copy(dest,src,count);
}

void Util::set(void *addr,int value,size_t count) {
	uchar *baddr;
	uint dwval = (value << 24) | (value << 16) | (value << 8) | value;
	uint *dwaddr = (uint*)addr;
	while(count >= sizeof(uint)) {
		*dwaddr++ = dwval;
		count -= sizeof(uint);
	}

	baddr = (uchar*)dwaddr;
	while(count-- > 0)
		*baddr++ = value;
}

void Util::copy(void *dest,const void *src,size_t len) {
	uchar *bdest,*bsrc;
	/* copy dwords first with loop-unrolling */
	uint *ddest = (uint*)dest;
	uint *dsrc = (uint*)src;
	while(len >= sizeof(uint) * 8) {
		*ddest = *dsrc;
		*(ddest + 1) = *(dsrc + 1);
		*(ddest + 2) = *(dsrc + 2);
		*(ddest + 3) = *(dsrc + 3);
		*(ddest + 4) = *(dsrc + 4);
		*(ddest + 5) = *(dsrc + 5);
		*(ddest + 6) = *(dsrc + 6);
		*(ddest + 7) = *(dsrc + 7);
		ddest += 8;
		dsrc += 8;
		len -= sizeof(uint) * 8;
	}
	/* copy remaining dwords */
	while(len >= sizeof(uint)) {
		*ddest++ = *dsrc++;
		len -= sizeof(uint);
	}

	/* copy remaining bytes */
	bdest = (uchar*)ddest;
	bsrc = (uchar*)dsrc;
	while(len-- > 0)
		*bdest++ = *bsrc++;
}
