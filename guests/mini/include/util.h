#pragma once

#include <stdint.h>

typedef unsigned long int size_t;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#define assert(x)
#define ARRAY_SIZE(a)	(sizeof((a)) / sizeof((a)[0]))
#define A_PACKED		__attribute__((packed))
#define A_ALIGNED(x)	__attribute__((aligned (x)))

class Util {
public:
	static inline void set_cr0(uint32_t val) {
		asm (
			"mov	%0,%%cr0"
			: : "r" (val)
		);
	}

	static inline uint32_t get_cr0() {
		uint32_t res;
		asm (
			"mov	%%cr0,%0"
				: "=r" (res)
		);
		return res;
	}

	static inline void set_cr3(uint32_t val) {
		asm (
			"mov	%0,%%cr3"
			: : "r" (val)
		);
	}

	static void copy(void* dst,const void* src,size_t len);
	static void set(void* addr,int value,size_t count);
	static void move(void* dest,const void* src,size_t count);

private:
	Util();
	~Util();
	Util(const Util&);
	Util& operator=(const Util&);
};
