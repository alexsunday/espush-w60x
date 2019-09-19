import os
import sys
import rtconfig

# set W60X_ROOT to W601_IoT_Board path.
if not os.getenv('W60X_ROOT'):
    print('Cannot found W60X_ROOT dir, please check W60X_ROOT')
    exit(-1)

W60X_ROOT = os.getenv('W60X_ROOT')
if not os.path.exists(W60X_ROOT):
    print('W60X_ROOT dir not exists, please check W60X_ROOT')
    print(W60X_ROOT)
    exit(-1)

RTT_ROOT = os.path.join(W60X_ROOT, "rt-thread")

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
try:
    from building import *
except:
    print('Cannot found RT-Thread root directory, please check RTT_ROOT')
    print(RTT_ROOT)
    exit(-1)

TARGET = 'rtthread.' + rtconfig.TARGET_EXT

env = Environment(tools = ['mingw'],
    AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
    CC = rtconfig.CC, CCFLAGS = rtconfig.CFLAGS,
    AR = rtconfig.AR, ARFLAGS = '-rc',
    LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)

if rtconfig.PLATFORM == 'iar':
    env.Replace(CCCOM = ['$CC $CCFLAGS $CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS -o $TARGET $SOURCES'])
    env.Replace(ARFLAGS = [''])
    env.Replace(LINKCOM = env["LINKCOM"] + ' --map project.map')

Export('RTT_ROOT')
Export('rtconfig')

# prepare building environment
objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)

# SDK_ROOT === W601_IoT_Board
SDK_ROOT = W60X_ROOT
bsp_vdir = 'build'

if os.path.exists(os.path.join(SDK_ROOT, 'libraries')):
    libraries_path_prefix = SDK_ROOT
else:
# include libraries and drivers
    libraries_path_prefix = os.path.dirname(os.path.dirname(SDK_ROOT))
objs.extend(SConscript(libraries_path_prefix + '/libraries/SConscript', variant_dir = bsp_vdir + '/libraries', duplicate = 0))
objs.extend(SConscript(libraries_path_prefix + '/drivers/SConscript', variant_dir = bsp_vdir + '/drivers', duplicate = 0))

# make a building
DoBuilding(TARGET, objs)
