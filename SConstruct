# -*- Mode: Python -*-

import sys, os

cross = 'i686-pc-nulnova'
crossver = '4.6.1'
crossdir = os.path.abspath('../cross/dist')

hostenv = Environment(
	CXXFLAGS = '-Wall -Wextra -ansi',
)
env = Environment(
	CXXFLAGS = '-Wall -Wextra -ansi',
	LINKFLAGS = '-Wl,--no-undefined -static -Wl,-static -static-libgcc',
	ENV = {'PATH' : crossdir + "/bin:" + os.environ['PATH']},
	CPPPATH = '#include',
	CXX = cross + '-g++',
	LD = cross + '-ld',
	AS = cross + '-as',
	CC = cross + '-gcc',
	AR = cross + '-ar',
	RANLIB = cross + '-ranlib',
)

verbose = ARGUMENTS.get('VERBOSE',0);
if int(verbose) == 0:
	hostenv['ASCOMSTR'] = env['ASCOMSTR'] = "AS $TARGET"
	hostenv['CCCOMSTR'] = env['CCCOMSTR'] = "CC $TARGET"
	hostenv['CXXCOMSTR'] = env['CXXCOMSTR'] = "CXX $TARGET"
	hostenv['LINKCOMSTR'] = env['LINKCOMSTR'] = "LD $TARGET"
	hostenv['ARCOMSTR'] = env['ARCOMSTR'] = "AR $TARGET"
	hostenv['RANLIBCOMSTR'] = env['RANLIBCOMSTR'] = "RANLIB $TARGET"

builddir = 'build'
debug = ARGUMENTS.get('debug', 0)
env.Append(CXXFLAGS = ' -O0 -g')
env.Append(CFLAGS = ' -O0 -g')
#env.Append(CXXFLAGS = ' -g -O3 -DNDEBUG')
#env.Append(CFLAGS = ' -g -O3 -DNDEBUG')

#if int(debug):
	#builddir = 'build/debug'
	#env.Append(CXXFLAGS = ' -O0')
#else:
	#builddir = 'build/release'
	#env.Append(CXXFLAGS = ' -O2')

env.Append(
	BUILDDIR = '#' + builddir,
	BINARYDIR = '#' + builddir + '/bin/apps',
	LIBPATH = '#' + builddir + '/bin/lib',
	SYSLIBPATH = crossdir + '/lib',
	SYSGCCLIBPATH = crossdir + '/lib/gcc/' + cross + '/' + crossver,
)

def NulProgram(env, target, source):
	prog = env.Program(
		target, source,
		LIBS = ['stdc++','supc++'],
		LIBPATH = [env['LIBPATH'], env['SYSLIBPATH']]
	)
	env.Depends(prog, env['SYSGCCLIBPATH'] + '/crt0.o')
	env.Depends(prog, env['SYSGCCLIBPATH'] + '/crt1.o')
	env.Depends(prog, env['SYSGCCLIBPATH'] + '/crtn.o')
	env.Depends(prog, env['LIBPATH'] + '/libstdc++.a')
	env.Depends(prog, env['LIBPATH'] + '/libc.a')
	env.Depends(prog, env['LIBPATH'] + '/libm.a')
	env.Depends(prog, env['LIBPATH'] + '/libg.a')
	env.Install(env['BINARYDIR'], prog)

env.NulProgram = NulProgram
hostenv.SConscript('tools/SConscript', 'hostenv', variant_dir = builddir + '/tools')
env.SConscript('libs/SConscript', 'env', variant_dir = builddir + '/libs')
env.SConscript('apps/SConscript', 'env', variant_dir = builddir + '/apps')
