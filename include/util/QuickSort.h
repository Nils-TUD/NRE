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

#include <util/Util.h>

namespace nul {

template<typename T>
class Quicksort {
public:
	typedef bool (*cmp_func)(T const &a,T const &b);

	static void sort(cmp_func cmp,T a[],size_t count) {
		sort(cmp,a,0,count - 1);
	}

private:
	static void sort(cmp_func cmp,T a[],size_t left,size_t right) {
		if(right > left) {
			size_t pivot_i = partition(cmp,a,left,right,(left + right) / 2);
			sort(cmp,a,left,pivot_i - 1);
			sort(cmp,a,pivot_i + 1,right);
		}
	}

	static int partition(cmp_func cmp,T a[],size_t left,size_t right,size_t pivot_i) {
		Util::swap<T>(a[pivot_i],a[right]);
		T &pivot = a[right];
		size_t si = left;

		for(size_t i = left; i < right; i++) {
			if(cmp(a[i],pivot))
				Util::swap<T>(a[i],a[si++]);
		}

		Util::swap<T>(a[si],a[right]);
		return si;
	}

	Quicksort();
};

}
