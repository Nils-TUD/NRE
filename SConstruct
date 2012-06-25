# -*- Mode: Python -*-

import sys, os

target = os.environ.get('NOVA_TARGET')
if target == 'x86_32':
	cross = 'i686-pc-nulnova'
elif target == 'x86_64':
	cross = 'x86_64-pc-nulnova'
else:
	print "Please define NOVA_TARGET to x86_32 or x86_64 first!"
	Exit(1)
crossver = '4.6.1'
crossdir = os.path.abspath('../cross/' + target + '/dist')

hostenv = Environment(
	ENV = os.environ,
	CXXFLAGS = '-Wall -Wextra -ansi'
)
env = Environment(
	CXXFLAGS = '-Wall -Wextra -ansi -mno-sse',
	CFLAGS = '-Wall -Wextra -mno-sse',
	# without "-z max-page-size=0x1000" ld produces really large binaries for x86_64, because it
	# aligns to 2MiB (not only in virtual memory, but also in the binary)
	LINKFLAGS = '-Wl,--no-undefined -static -Wl,-static -static-libgcc -Wl,-z,max-page-size=0x1000',
	ENV = {'PATH' : crossdir + "/bin:" + os.environ['PATH']},
	CPPPATH = '#include',
	CXX = cross + '-g++',
	LD = cross + '-ld',
	AS = cross + '-as',
	CC = cross + '-gcc',
	AR = cross + '-ar',
	RANLIB = cross + '-ranlib',
)

btype = os.environ.get('NOVA_BUILD')
if btype == 'debug':
	env.Append(CXXFLAGS = ' -O0 -g')
	env.Append(CFLAGS = ' -O0 -g')
else:
	# we enable the framepointer to get stacktraces in release-mode. of course, we could also
	# disable stacktraces later, so that we don't need the framepointer.
	env.Append(CXXFLAGS = ' -O3 -DNDEBUG -fno-omit-frame-pointer')
	env.Append(CFLAGS = ' -O3 -DNDEBUG -fno-omit-frame-pointer')
	btype = 'release'
builddir = 'build/' + target + '-' + btype

verbose = ARGUMENTS.get('VERBOSE',0);
if int(verbose) == 0:
	hostenv['ASCOMSTR'] = env['ASCOMSTR'] = "AS $TARGET"
	hostenv['CCCOMSTR'] = env['CCCOMSTR'] = "CC $TARGET"
	hostenv['CXXCOMSTR'] = env['CXXCOMSTR'] = "CXX $TARGET"
	hostenv['LINKCOMSTR'] = env['LINKCOMSTR'] = "LD $TARGET"
	hostenv['ARCOMSTR'] = env['ARCOMSTR'] = "AR $TARGET"
	hostenv['RANLIBCOMSTR'] = env['RANLIBCOMSTR'] = "RANLIB $TARGET"

env.Append(
	ARCH = target,
	ROOTDIR = '#',
	BUILDDIR = '#' + builddir,
	BINARYDIR = '#' + builddir + '/bin/apps',
	LIBPATH = '#' + builddir + '/bin/lib',
	SYSLIBPATH = crossdir + '/lib',
	SYSGCCLIBPATH = crossdir + '/lib/gcc/' + cross + '/' + crossver,
	LINK_ADDR = 0x100000
)

def NulProgram(env, target, source):
	# for better debugging: put every executable at a different address
	myenv = env.Clone()
	myenv.Append(LINKFLAGS = ' -Wl,--section-start=.init=' + ("0x%x" % env['LINK_ADDR']))
	env['LINK_ADDR'] += 0x100000
	prog = myenv.Program(
		target, source,
		LIBS = ['stdc++','supc++'],
		LIBPATH = [myenv['LIBPATH'], myenv['SYSLIBPATH']]
	)
	myenv.Depends(prog, myenv['SYSGCCLIBPATH'] + '/crt0.o')
	myenv.Depends(prog, myenv['SYSGCCLIBPATH'] + '/crt1.o')
	myenv.Depends(prog, myenv['SYSGCCLIBPATH'] + '/crtn.o')
	myenv.Depends(prog, myenv['LIBPATH'] + '/libstdc++.a')
	myenv.Depends(prog, myenv['LIBPATH'] + '/libc.a')
	myenv.Depends(prog, myenv['LIBPATH'] + '/libm.a')
	myenv.Depends(prog, myenv['LIBPATH'] + '/libg.a')
	myenv.Install(env['BINARYDIR'], prog)

env.NulProgram = NulProgram
hostenv.SConscript('tools/SConscript', 'hostenv', variant_dir = builddir + '/tools')
env.SConscript('libs/SConscript', 'env', variant_dir = builddir + '/libs')
env.SConscript('apps/SConscript', 'env', variant_dir = builddir + '/apps')
