# -*- Mode: Python -*-

import sys, os

cross = 'i686-pc-nulnova'
crossver = '4.6.1'
#crossver = '4.4.3'
crossdir = os.path.abspath('../cross/dist')
#crossdir = os.path.abspath('../cross4.4.3/dist')

env = Environment(
	CXXFLAGS = '-Wall -Weffc++ -Wextra -ansi -g',
	LINKFLAGS = '-static -Wl,-static -static-libgcc -g',
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
	env['ASCOMSTR'] = "AS $TARGET"
	env['CXXCOMSTR'] = "CXX $TARGET"
	env['LINKCOMSTR'] = "LD $TARGET"
	env['ARCOMSTR'] = "AR $TARGET"
	env['RANLIBCOMSTR'] = "RANLIB $TARGET"

builddir = 'build'
debug = ARGUMENTS.get('debug', 0)
env.Append(CXXFLAGS = ' -O0')
env.Append(CFLAGS = ' -O0')

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
		LIBS = 'supc++',
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
env.SConscript('libs/SConscript', 'env', variant_dir = builddir + '/libs')
env.SConscript('apps/SConscript', 'env', variant_dir = builddir + '/apps')
