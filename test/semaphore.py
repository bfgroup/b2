#!/usr/bin/env python3

# Copyright 2026 Paolo Pastori
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)

# Test for the JAM_SEMAPHORE variable:
# verify that actions for targets sharing the same named semaphore
# cannot be run in parallel.

# NOTE: because of the jam update action calling this python script
#       (actually a copy), this test does not work on non-posix platforms,
#       this limitation is primarily due to different launchers, python
#       interpreter/shell conventions to be used on non posix platform
#       (e.g. windows) of which b2 is not aware of.

import sys

def touch(fname):
    with open(fname, 'w'):
        pass

def update(sentry, secs, target):
    from os.path import isfile
    if isfile(sentry): print('PARALLEL UPDATE')
    else: touch(sentry)
    from time import sleep
    sleep(int(secs))
    touch(target)
    from os import unlink
    try:
        unlink(sentry)
    except FileNotFoundError:
        print('PARALLEL UPDATE')
    except:
        pass

if len(sys.argv) == 4:
    # called by jam udate action
    # ./semaphore.py <sentry_fname> <sleep_secs> <target_fname>
    update(*sys.argv[1:])
    sys.exit()

import os.path
# remember this script absolute pathname
script = os.path.abspath(__file__)

import BoostBuild
import shutil

t = BoostBuild.Tester(['-ffile.jam', '-j2'], pass_toolset=False)

# install in workdir a copy of this script
shutil.copy(script, 'semaphore.py')

# 1. test parallel execution of update
t.write('file.jam', '''\
DEPENDS all : x1 x2 ;
actions update
{
    "./semaphore.py" sentry 1 $(<)
}

update x1 ;
update x2 ;
''')

t.run_build_system()
t.expect_addition('x1')
t.expect_addition('x2')
t.expect_output_lines('PARALLEL UPDATE')

t.rm('x1')
t.rm('x2')

# 2. test parallel execution suppression by JAM_SEMAPHORE
t.write('file.jam', '''\
DEPENDS all : x1 x2 ;
actions update
{
    "./semaphore.py" sentry 1 $(<)
}

JAM_SEMAPHORE on x1 x2 = <s>update_sem ;

update x1 ;
update x2 ;
''')

expected_output = '''\
...found 3 targets...
...updating 2 targets...
update x1
update x2

...updated 2 targets...
'''

t.run_build_system(stdout=expected_output)
t.expect_addition('x1')
t.expect_addition('x2')

t.cleanup()
