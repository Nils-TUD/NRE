# -*- Mode: Python -*-

import sys, os

cross = 'i686-pc-nulnova'
crossver = '4.6.1'
crossdir = os.path.abspath('../cross/dist')

sys.path.append(crossdir + "/bin")
env = Environment(
	CXXFLAGS = '-Wall -Weffc++ -Wextra -ansi -g',
	LINKFLAGS = '-Wl,-static -static-libgcc -g',
	ENV = {'PATH' : os.environ['PATH']},
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
if int(debug):
	#builddir = 'build/debug'
	env.Append(CXXFLAGS = ' -O0')
else:
	#builddir = 'build/release'
	env.Append(CXXFLAGS = ' -O2')

env.Append(
	LIBPATH = '#' + builddir + '/bin/lib',
	SYSLIBPATH = crossdir + '/lib',
	SYSGCCLIBPATH = crossdir + '/lib/gcc/' + cross + '/' + crossver,
	BINARYDIR = '#' + builddir + '/bin/apps',
	BUILDDIR = '#' + builddir,
)

env.SConscript('libs/SConscript', 'env', variant_dir = builddir + '/libs')
env.SConscript('apps/SConscript', 'env', variant_dir = builddir + '/apps')
