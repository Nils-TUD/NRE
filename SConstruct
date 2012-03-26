# -*- Mode: Python -*-

import sys, os

cross = 'i686-pc-nulnova'
crossver = '4.6.1'
crossdir = os.path.abspath('../cross/dist')

sys.path.append(crossdir + "/bin")
env = Environment(
	CXXFLAGS = '-Wall -Weffc++ -Wextra -ansi',
	LINKFLAGS = '-static',
	ENV = {'PATH' : os.environ['PATH']},
	#CXXCOMSTR = "CXX $TARGET", LINKCOMSTR = "LD $TARGET",
	CXX = cross + '-g++',
	LD = cross + '-ld',
	AS = cross + '-as',
	CC = cross + '-gcc',
	AR = cross + '-ar',
	RANLIB = cross + '-ranlib',
)

debug = ARGUMENTS.get('debug', 0)
if int(debug):
	builddir = 'build/debug'
	env.Append(CCFLAGS = '-g -O0')
else:
	builddir = 'build/release'
	env.Append(CXXFLAGS = '-g -O2')

env.Append(
	LIBPATH = '#' + builddir,
	SYSLIBPATH = crossdir + '/lib/gcc/' + cross + '/' + crossver,
	BINARYDIR = '#' + builddir,
	BUILDDIR = '#' + builddir,
)

env.SConscript('libs/SConscript', 'env', variant_dir = builddir + '/libs')
env.SConscript('apps/SConscript', 'env', variant_dir = builddir + '/apps')

