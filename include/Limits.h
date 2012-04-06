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

#define CHAR_BIT		8
#define CHAR_MAX		SCHAR_MAX
#define CHAR_MIN		SCHAR_MIN
#define SCHAR_MAX		127
#define SCHAR_MIN		(-128)
#define SHRT_MAX		32767
#define SHRT_MIN		(-32768)
#define INT_MAX			32767
#define INT_MIN			(-32768)
#define LONG_MAX		2147483647LL
#define LONG_MIN		(-2147483648LL)
#define LLONG_MAX		9223372036854775807LL
#define LLONG_MIN		(-LLONG_MAX - 1LL)
#define UCHAR_MAX		255
#define USHRT_MAX		65535
#define UINT_MAX		65535
#define ULONG_MAX		4294967295ULL
#define ULLONG_MAX		18446744073709551615ULL

template<typename T>
struct numeric_limits {
	/** This will be true for all fundamental types (which have
	 specializations), and false for everything else.  */
	static const bool is_specialized = false;

	/** The number of @c radix digits that be represented without change:  for
	 integer types, the number of non-sign bits in the mantissa; for
	 floating types, the number of @c radix digits in the mantissa.  */
	static const int digits = 0;
	/** The number of base 10 digits that can be represented without change. */
	static const int digits10 = 0;
	/** True if the type is signed.  */
	static const bool is_signed = false;
	/** True if the type is integer.
	 *  Is this supposed to be "if the type is integral"?
	 */
	static const bool is_integer = false;
	/** True if the type uses an exact representation.  "All integer types are
	 exact, but not all exact types are integer.  For example, rational and
	 fixed-exponent representations are exact but not integer."
	 [18.2.1.2]/15  */
	static const bool is_exact = false;
	/** For integer types, specifies the base of the representation.  For
	 floating types, specifies the base of the exponent representation.  */
	static const int radix = 0;

	/** The minimum finite value, or for floating types with
	 denormalization, the minimum positive normalized value.  */
	static T min() throw () {
		return static_cast<T> (0);
	}
	/** The maximum finite value.  */
	static T max() throw () {
		return static_cast<T> (0);
	}
};

template<>
struct numeric_limits<bool> {
	static const bool is_specialized = true;

	static bool min() throw () {
		return false;
	}
	static bool max() throw () {
		return true;
	}

	static const int digits = 1;
	static const int digits10 = 0;
	static const bool is_signed = false;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<char> {
	static const bool is_specialized = true;

	static char min() throw () {
		return SCHAR_MIN;
	}
	static char max() throw () {
		return SCHAR_MAX;
	}

	static const int digits = 7;
	static const int digits10 = 3;
	static const bool is_signed = true;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<unsigned char> {
	static const bool is_specialized = true;

	static unsigned char min() throw () {
		return 0;
	}
	static unsigned char max() throw () {
		return UCHAR_MAX;
	}

	static const int digits = 8;
	static const int digits10 = 3;
	static const bool is_signed = false;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<short> {
	static const bool is_specialized = true;

	static short min() throw () {
		return SHRT_MIN;
	}
	static short max() throw () {
		return SHRT_MAX;
	}

	static const int digits = 15;
	static const int digits10 = 5;
	static const bool is_signed = true;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<unsigned short> {
	static const bool is_specialized = true;

	static unsigned short min() throw () {
		return 0;
	}
	static unsigned short max() throw () {
		return USHRT_MAX;
	}

	static const int digits = 16;
	static const int digits10 = 5;
	static const bool is_signed = false;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<int> {
	static const bool is_specialized = true;

	static int min() throw () {
		return INT_MIN;
	}
	static int max() throw () {
		return INT_MAX;
	}

	static const int digits = 15;
	static const int digits10 = 5;
	static const bool is_signed = true;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<unsigned int> {
	static const bool is_specialized = true;

	static unsigned int min() throw () {
		return 0;
	}
	static unsigned int max() throw () {
		return UINT_MAX;
	}

	static const int digits = 16;
	static const int digits10 = 5;
	static const bool is_signed = false;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<long> {
	static const bool is_specialized = true;

	static long min() throw () {
		return LONG_MIN;
	}
	static long max() throw () {
		return LONG_MAX;
	}

	static const int digits = 31;
	static const int digits10 = 10;
	static const bool is_signed = true;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<unsigned long> {
	static const bool is_specialized = true;

	static unsigned long min() throw () {
		return 0;
	}
	static unsigned long max() throw () {
		return ULONG_MAX;
	}

	static const int digits = 32;
	static const int digits10 = 10;
	static const bool is_signed = false;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<long long> {
	static const bool is_specialized = true;

	static long long min() throw () {
		return LLONG_MIN;
	}
	static long long max() throw () {
		return LLONG_MAX;
	}

	static const int digits = 63;
	static const int digits10 = 19;
	static const bool is_signed = true;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};

template<>
struct numeric_limits<unsigned long long> {
	static const bool is_specialized = true;

	static unsigned long long min() throw () {
		return 0;
	}
	static unsigned long long max() throw () {
		return ULLONG_MAX;
	}

	static const int digits = 64;
	static const int digits10 = 20;
	static const bool is_signed = false;
	static const bool is_integer = true;
	static const bool is_exact = true;
	static const int radix = 2;
};
