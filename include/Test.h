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

#include <stream/OStringStream.h>
#include <stream/Log.h>
#include <utcb/UtcbFrame.h>
#include <Errors.h>

namespace nul {
namespace test {

typedef void (*test_func)();
enum {
	MAX_FUNCS = 32
};

struct TestCase {
	const char *name;
	test_func func;
};

// Whether to print info before execution of the test. Zero means
// print info after execution together with results.
#define WVTEST_PRINT_INFO_BEFORE 0

// Standard WVTEST API
#define WVSTART(title) Log::get().writef("Testing \"%s\" in %s:%d:\n", title, WvTest::repo_rel_path(__FILE__), __LINE__);
#define WVPASS(cond)   ({ WvTest __t(__FILE__, __LINE__, #cond);            __t.check(cond); })
#define WVNOVA(novaerr)({ WvTest __t(__FILE__, __LINE__, #novaerr);         __t.check_novaerr(novaerr); })
#define WVPASSEQ(a, b) ({ WvTest __t(__FILE__, __LINE__, #a " == " #b);     __t.check_eq((a), (b), true); })
#define WVPASSLT(a, b) ({ WvTest __t(__FILE__, __LINE__, #a " < " #b);      __t.check_lt((a), (b)); })
#define WVPASSGE(a, b) ({ WvTest __t(__FILE__, __LINE__, #a " >= " #b);     __t.check_le((b), (a)); })
#define WVFAIL(cond)   ({ WvTest __t(__FILE__, __LINE__, "NOT(" #cond ")"); !__t.check(!(cond)); })
#define WVFAILEQ(a, b) ({ WvTest __t(__FILE__, __LINE__, #a " != " #b);     __t.check_eq((a), (b), false); })
#define WVPASSNE(a, b) WVFAILEQ(a, b)
#define WVFAILNE(a, b) WVPASSEQ(a, b)

// Performance monitoring
#define WVPERF(value, units)  ({ WvTest __t(__FILE__, __LINE__, "PERF: " #value);  __t.check_perf(value, units); })

// Debugging
#define WV(code)    ({ WvTest __t(__FILE__, __LINE__, #code); __t.check(true); code; })
#define WVSHOW(val) ({ WvTest __t(__FILE__, __LINE__, #val);  __t.show(val); })
#define WVSHOWHEX(val)  ({ WvTest __t(__FILE__, __LINE__, #val);  __t.show_hex(val); })
#define WVPRINTF(fmt, ...)  Log::get().writef("! %s:%d " fmt " ok\n", WvTest::repo_rel_path(__FILE__), __LINE__, ##__VA_ARGS__)

class WvTest {
	const char *file,*condstr;
	int line;

	struct NovaErr {
		enum ErrorCode err;
		NovaErr(unsigned char _err) :
				err(static_cast<enum ErrorCode>(_err)) {
		}

		const char *tostr() {
			switch(err) {
				case ESUCCESS: return "ok";
				case ETIMEOUT: return "ETIMEOUT";
				case EABORT: return "EABORT";
				case ESYS: return "ESYS";
				case ECAP: return "ECAP";
				case EMEM: return "EMEM";
				case EFTR: return "EFTR";
				case ECPU: return "ECPU";
				case EDEV: return "EDEV";
			}
			char *ret = new char[30]; // XXX memory leak
			OStringStream::format(ret,30,"ERR:0x%x",err);
			return ret;
		}
	};

	const char *resultstr(bool result) {
		return result ? "ok" : "FAILED";
	}

	const char *resultstr(char *result) {
		return result;
	}

	const char *resultstr(NovaErr result) {
		return result.tostr();
	}

	void save_info(const char *_file,int _line,const char *_condstr) {
		file = repo_rel_path(_file);
		condstr = _condstr;
		line = _line;
	}

#if WVTEST_PRINT_INFO_BEFORE
	void print_info()
	{	Log::get().writef("! %s:%d %s ", file, line, condstr);}

	template <typename T>
	void print_result(T result, const char* suffix="", const char *sb="", const char *se="")
	{	Log::get().writef("%s%s%s %s\n", sb, suffix, se, resultstr(result));}
#else
	template<typename T>
	void print_result(T result,const char* suffix = "",const char *sb = "",const char *se = "") {
		Log::get().writef("! %s:%d %s %s%s%s %s\n",file,line,condstr,sb,suffix,se,resultstr(result));
	}
#endif

	static void print_failed_cmp(const char *op,const char *a,const char *b) {
		Log::get().writef("wvtest comparison '%s' %s '%s' FAILED\n",a,op,b);
	}

	static void print_failed_cmp(const char *op,unsigned a,unsigned b) {
		Log::get().writef("wvtest comparison %d == 0x%x %s %d == 0x%x FAILED\n",a,a,op,b,b);
	}

	static void print_failed_cmp(const char *op,ulong a,ulong b) {
		Log::get().writef("wvtest comparison %ld == 0x%lx %s %ld == 0x%lx FAILED\n",a,a,op,b,b);
	}

	static void print_failed_cmp(const char *op,ullong a,ullong b) {
		Log::get().writef("wvtest comparison %Ld == 0x%Lx %s %Ld == 0x%Lx FAILED\n",a,a,op,b,b);
	}

	static void print_failed_cmp(const char *op,int a,int b) {
		Log::get().writef("wvtest comparison %d == 0x%x %s %d == 0x%x FAILED\n",a,a,op,b,b);
	}

	static void stringify(char *buf,unsigned size,ullong val) {
		OStringStream::format(buf,size,"%Lu",val);
	}

	static void stringify(char *buf,unsigned size,ulong val) {
		OStringStream::format(buf,size,"%lu",val);
	}

	static void stringify(char *buf,unsigned size,unsigned val) {
		OStringStream::format(buf,size,"%u",val);
	}

	static void stringify(char *buf,unsigned size,int val) {
		OStringStream::format(buf,size,"%d",val);
	}

	static void stringify(char *buf,unsigned size,Crd crd) {
		switch(crd.value() & 0x3) {
			case 0:
				OStringStream::format(buf,size,"CRD(0)");
				return;
			case DESC_TYPE_MEM: {
				char r = crd.value() & 0x04 ? 'r' : '-';
				char w = crd.value() & 0x08 ? 'w' : '-';
				char x = crd.value() & 0x10 ? 'x' : '-';
				OStringStream::format(
					buf,size,"CRD(mem, 0x%x+0x%x, %c%c%c)",crd.base(),crd.size(),r,w,x
				);
				return;
			}
			case DESC_TYPE_IO:
				OStringStream::format(buf,size,"CRD(io, 0x%x, 2^%d)",crd.base(),crd.order());
				return;
			case DESC_TYPE_CAP:
				OStringStream::format(
					buf,size,"CRD(obj, 0x%x, 2^%d, 0x%x)",crd.cap(),crd.order(),crd.value() >> 2 & 0x1f
				);
				return;
		}
	}

	static void stringifyx(char *buf,unsigned size,ullong val) {
		OStringStream::format(buf,size,"0x%Lx",val);
	}

	static void stringifyx(char *buf,unsigned size,ulong val) {
		OStringStream::format(buf,size,"0x%lx",val);
	}

	static void stringifyx(char *buf,unsigned size,unsigned val) {
		OStringStream::format(buf,size,"0x%x",val);
	}

	static void stringifyx(char *buf,unsigned size,int val) {
		OStringStream::format(buf,size,"0x%x",val);
	}

	static void stringifyx(char *buf,unsigned size,void *val) {
		OStringStream::format(buf,size,"%p",val);
	}

public:
	static const char *repo_rel_path(const char *filename) {
		const char wvtest_h[] = __FILE__;
		const char name_in_repo[] = "michal/include/wvtest.h";
		size_t rel_idx = sizeof(wvtest_h) - sizeof(name_in_repo);

		const char *p1 = wvtest_h + rel_idx;
		const char *p2 = name_in_repo;
		while(*p1 && *p2)
			if(*p1++ != *p2++) // Unexpected location of this file
				return filename; // Return absolute path

		for(unsigned i = 0; i < rel_idx; i++)
			if(filename[i] != wvtest_h[i])
				rel_idx = 0;
		// Return repo-relative path if the 'filename' is in repo.
		return filename + rel_idx;
	}

	WvTest(const char *file,int line,const char *condstr) {
		save_info(file,line,condstr);
#if WVTEST_PRINT_INFO_BEFORE
		// If we are sure that nothing is printed during the "check", we can
		// print the info here, and the result after the "check" finishes.
		print_info(true, file, line, condstr, 0);
#endif
	}

	bool check(bool cond,const char* suffix = "") {
		print_result(cond,suffix);
		return cond;
	}

	unsigned check_novaerr(unsigned novaerr) {
		print_result(NovaErr(novaerr));
		return novaerr;
	}

	bool check_eq(const char *a,const char *b,bool expect_equal) {
		bool result = !!strcmp(a,b) ^ expect_equal;
		if(!result)
			print_failed_cmp(expect_equal ? "==" : "!=",a,b);
		check(result);
		return result;
	}
	bool check_eq(char *a,char *b,bool expect_equal) {
		return check_eq(static_cast<const char*>(a),static_cast<const char*>(b),expect_equal);
	}
	bool check_eq(const char *a,char *b,bool expect_equal) {
		return check_eq(static_cast<const char*>(a),static_cast<const char*>(b),expect_equal);
	}
	bool check_eq(char *a,const char *b,bool expect_equal) {
		return check_eq(static_cast<const char*>(a),static_cast<const char*>(b),expect_equal);
	}

	template<typename T>
	bool check_eq(T a,T b,bool expect_equal) {
		bool result = ((a == b) ^ !expect_equal);
		if(!result)
			print_failed_cmp(expect_equal ? "==" : "!=",a,b);
		check(result);
		return result;
	}

	template<typename T>
	bool check_lt(T a,T b) {
		bool result = (a < b);
		if(!result)
			print_failed_cmp("<",a,b);
		check(result);
		return result;
	}

	template<typename T>
	bool check_le(T a,T b) {
		bool result = (a <= b);
		if(!result)
			print_failed_cmp("<=",a,b);
		check(result);
		return result;
	}

	template<typename T>
	T check_perf(T val,const char *units) {
		char valstr[20 + strlen(units)];
		stringify(valstr,sizeof(valstr),val);
		print_result(true," ",valstr,units);
		return val;
	}

	const char *show(const char *val) {
		print_result(true,val,"= \"","\"");
		return val;
	}

	char *show(char *val) {
		print_result(true,val,"= \"","\"");
		return val;
	}

	template<typename T>
	T show(T val) {
		char valstr[40];
		stringify(valstr,sizeof(valstr),val);
		print_result(true,valstr,"= ");
		return val;
	}

	template<typename T>
	T show_hex(T val) {
		char valstr[40];
		stringifyx(valstr,sizeof(valstr),val);
		print_result(true,valstr,"= ");
		return val;
	}
};

}
}
