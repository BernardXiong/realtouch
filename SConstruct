import os
import sys
import rtconfig

if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')
else:
    RTT_ROOT = os.path.normpath(os.getcwd() + '/../..')

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
from building import *

TARGET = 'rtthread-stm32f4xx.' + rtconfig.TARGET_EXT

env = Environment(tools = ['mingw'],
	AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
	CC = rtconfig.CC, CCFLAGS = rtconfig.CFLAGS,
	AR = rtconfig.AR, ARFLAGS = '-rc',
	LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)

def mdk_create_tmpfile(tmpfile, objs):
    cmdline =''
    tmpfile = file(tmpfile, 'w')
    for item in objs:
        # print type(item), os.path.basename(str(item))
        cmdline += os.path.normpath(str(item))
        cmdline += ' '

    tmpfile.write(cmdline)
    tmpfile.close();
    return

if rtconfig.PLATFORM == 'armcc':
    env["LINKCOM"] = "$LINK -o $TARGET $LINKFLAGS --via tmpcmd.txt"

Export('RTT_ROOT')
Export('rtconfig')

# prepare building environment
objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)

# build program
if rtconfig.PLATFORM == 'armcc':
	mdk_create_tmpfile('tmpcmd.txt', objs)

# make a building
DoBuilding(TARGET, objs)
