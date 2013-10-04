#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import commands
import string

EnsureSConsVersion(0, 98, 1)

PACKAGE = 'airdcpp'
CORE_PACKAGE = 'libdcpp'
BUILD_PATH = '#/build/'

BUILD_FLAGS = {
	'common'  : ['-I#', '-D_GNU_SOURCE', '-D_LARGEFILE_SOURCE', '-D_FILE_OFFSET_BITS=64', '-D_REENTRANT', '-D__cdecl=""', '-std=c++11', '-Wfatal-errors', '-fexceptions', '-Wno-overloaded-virtual'],
	'debug'   : ['-g', '-ggdb', '-Wall', '-D_DEBUG', '-Wno-reorder' ], 
	'release' : ['-O3', '-fomit-frame-pointer', '-DNDEBUG']
}

# ----------------------------------------------------------------------
# Function definitions
# ----------------------------------------------------------------------

def check_pkg_config(context):
	context.Message('Checking for pkg-config... ')
	ret = context.TryAction('pkg-config --version')[0]
	context.Result(ret)
	return ret

def check_pkg(context, name):
	context.Message('Checking for %s... ' % name)
	ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
	context.Result(ret)
	return ret

def check_cxx_version(context, name, major, minor):
	context.Message('Checking for %s >= %d.%d...' % (name, major, minor))
	ret = commands.getoutput('%s -dumpversion' % name)

	retval = 0
	try:
		if ((string.atoi(ret[0]) == major and string.atoi(ret[2]) >= minor)
		or (string.atoi(ret[0]) > major)):
			retval = 1
	except ValueError:
		print "No C++ compiler found!"

	context.Result(retval)
	return retval

# Recursively installs all files within the source folder to target. Optionally,
# a filter function can be provided to prevent installation of certain files.
def recursive_install(env, source, target, filter = None):
	nodes = env.Glob(os.path.join(source, '*'))
	target = os.path.join(target, os.path.basename(source))

	for node in nodes:
		if node.isdir():
			env.RecursiveInstall(str(node), target, filter)
		elif filter == None or filter(node.name):
			env.Alias('install', env.Install(target, node))


# ----------------------------------------------------------------------
# Command-line options
# ----------------------------------------------------------------------

# Parameters are only sticky from scons -> scons install, otherwise they're cleared.
if 'install' in COMMAND_LINE_TARGETS:
	vars = Variables('build/sconf/scache.conf')
else:
	vars = Variables()

vars.AddVariables(
	BoolVariable('debug', 'Compile the program with debug information', 0),
	BoolVariable('release', 'Compile the program with optimizations', 0),
	BoolVariable('profile', 'Compile the program with profiling information', 0),
	PathVariable('PREFIX', 'Compile the program with PREFIX as the root for installation', '/usr/local', PathVariable.PathIsDir),
	('FAKE_ROOT', 'Make scons install the program under a fake root', '')
)


# ----------------------------------------------------------------------
# Initialization
# ----------------------------------------------------------------------

env = Environment(ENV = os.environ, variables = vars, package = PACKAGE)

env['mode'] = 'debug' if env.get('debug') else 'release'
env['build_path'] = BUILD_PATH + env['mode'] + '/'

if os.environ.has_key('CXX'):
	env['CXX'] = os.environ['CXX']
else:
	print 'CXX env variable is not set, attempting to use g++'
	env['CXX'] = 'g++'

if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']

if os.environ.has_key('CXXFLAGS'):
	env['CPPFLAGS'] = env['CXXFLAGS'] = os.environ['CXXFLAGS'].split()

if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] = os.environ['LDFLAGS'].split()

if os.environ.has_key('CFLAGS'):
	env['CFLAGS'] = os.environ['CFLAGS'].split()

env['CPPDEFINES'] = [] # Initialize as a list so Append doesn't concat strings

env.SConsignFile('build/sconf/.sconsign')
vars.Save('build/sconf/scache.conf', env)


env.AddMethod(recursive_install, 'RecursiveInstall')

conf = env.Configure(
	custom_tests =
	{
		'CheckPKGConfig' : check_pkg_config,
		'CheckPKG' : check_pkg,
		'CheckCXXVersion' : check_cxx_version,
	},
	conf_dir = 'build/sconf',
	log_file = 'build/sconf/config.log')


# ----------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------

if not 'install' in COMMAND_LINE_TARGETS:

	# Dependencies
	if env['CXX'] == 'clang':
		if not conf.CheckCXXVersion(env['CXX'], 3, 3):
			 print 'Compiler version check failed. Supported compilers: clang 3.3 or later, g++ 4.7 or later'
			 Exit(1)

	elif not conf.CheckCXXVersion(env['CXX'], 4, 7):
		 print 'Compiler version check failed. Supported compilers: clang 3.3 or later, g++ 4.7 or later'
		 Exit(1)
	
	if not conf.CheckCXXHeader('boost/version.hpp', '<>'):
		print '\tboost not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('leveldb', 'leveldb/db.h', 'cpp'):
		print '\tleveldb not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('GeoIP', 'GeoIP.h', 'c'):
		print '\tGeoIP not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('tbb', 'tbb/tbb.h', 'cpp'):
		print '\tTBB not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('natpmp', 'natpmp.h', 'c'):
		print '\tnatpmp not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('miniupnpc', 'miniupnpc/miniupnpc.h', 'c'):
		print '\tminiupnpc not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLib('boost_regex'):
		print '\tboost_regex not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLib('boost_system'):
		print '\tboost_system not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLib('boost_thread'):
		print '\tboost_thread not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckHeader('time.h'):
		Exit(1)

	if not conf.CheckHeader('signal.h'):
		Exit(1)

	if not conf.CheckHeader('unistd.h'):
		Exit(1)

	if not conf.CheckLibWithHeader('pthread', 'pthread.h', 'c'):
		print '\tpthread library not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('z', 'zlib.h', 'c'):
		print '\tz library (gzip/z compression) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('bz2', 'bzlib.h', 'c'):
		print '\tbz2 library (bz2 compression) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	# This needs to be before ssl check on *BSD systems
	if not conf.CheckLib('crypto'):
		print '\tcrypto library not found'
		print '\tNote: This library may be a part of libssl on your system'
		Exit(1)

	if not conf.CheckLibWithHeader('ssl', 'openssl/ssl.h', 'c'):
		print '\tOpenSSL library (libssl) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)
		
	if not conf.CheckLib('stdc++'):
		print '\tstdc++ library not found'
		Exit(1)

	if not conf.CheckLib('m'):
		print '\tmath library not found'
		Exit(1)


	if conf.CheckHeader(['sys/types.h', 'sys/socket.h', 'ifaddrs.h', 'net/if.h']):
		conf.env.Append(CPPDEFINES = 'HAVE_IFADDRS_H')

	env = conf.Finish()


# ----------------------------------------------------------------------
# Compile and link flags
# ----------------------------------------------------------------------

	env.MergeFlags(BUILD_FLAGS['common'])
	env.MergeFlags(BUILD_FLAGS[env['mode']])

	env.Append(LIBPATH = env['build_path'] + CORE_PACKAGE)
	env.Prepend(LIBS = 'client')

	if os.sys.platform == 'linux2':
		env.Append(LINKFLAGS = '-Wl,--as-needed')

	if os.name == 'mac' or os.sys.platform == 'darwin':
		conf.env.Append(CPPDEFINES = ('ICONV_CONST', ''))

	if os.sys.platform == 'sunos5':
		conf.env.Append(CPPDEFINES = ('ICONV_CONST', 'const'))
		env.Append(LIBS = ['socket', 'nsl'])

	if env.get('profile'):
		env.Append(CXXFLAGS = '-pg')
		env.Append(LINKFLAGS= '-pg')

	if env.get('PREFIX'):
		data_dir = '\'\"%s/share\"\'' % env['PREFIX']
		env.Append(CPPDEFINES = ('_DATADIR', data_dir))


# ----------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------

	# Build the dcpp library
	dcpp_env = env.Clone(package = CORE_PACKAGE)
	libdcpp = SConscript(dirs = 'client', variant_dir = env['build_path'] + CORE_PACKAGE, duplicate = 0, exports = {'env': dcpp_env})

	ui_env = env.Clone()

	obj_files = SConscript(dirs = 'linux', variant_dir = env['build_path'] + 'gui', duplicate = 0, exports = {'env': ui_env})

	# Create the executable
	env.Program(target = PACKAGE, source = [libdcpp, obj_files])

	# Build source files followed by everything else
	Default(PACKAGE, '.')


# ----------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------

else:

	#text_files = env.Glob('*.txt')
	prefix = env['FAKE_ROOT'] + env['PREFIX']
	#desktop_file = os.path.join('data', PACKAGE + '.desktop')
	#app_icon_filter = lambda icon: os.path.splitext(icon)[0] == PACKAGE
	#regular_icon_filter = lambda icon: os.path.splitext(icon)[0] != PACKAGE

	#env.Alias('install', env.Install(dir = os.path.join(prefix, 'share', PACKAGE, 'glade'), source = glade_files))
	#env.Alias('install', env.Install(dir = os.path.join(prefix, 'share', 'doc', PACKAGE), source = text_files))
	#env.Alias('install', env.Install(dir = os.path.join(prefix, 'share', 'applications'), source = desktop_file))
	env.Alias('install', env.Install(dir = os.path.join(prefix, 'bin'), source = PACKAGE))

