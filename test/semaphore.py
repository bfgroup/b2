#!/usr/bin/env python3

# Copyright 2026 Paolo Pastori
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)

# Test for the JAM_SEMAPHORE variable:
# verify that actions for targets sharing the same named semaphore
# cannot be run in parallel.

import sys

def touch(fname):
    with open(fname, 'w'):
        pass

if len(sys.argv) == 4:
    # called with <sentry_fname> <sleep_secs> <target_fname>
    from os.path import isfile
    if isfile(sys.argv[1]): print('PARALLEL SLEEP')
    else: touch(sys.argv[1])
    from time import sleep
    sleep(int(sys.argv[2]))
    touch(sys.argv[3])
    from os import unlink
    try:
        unlink(sys.argv[1])
    except FileNotFoundError:
        print('PARALLEL SLEEP')
    except:
        pass
    sys.exit()

import os.path
# remember this script absolute pathname
script = os.path.abspath(__file__)

import BoostBuild

t = BoostBuild.Tester(['-ffile.jam', '-j2'], pass_toolset=False)

# First test parallel execution of update
t.write('file.jam', """\
DEPENDS all : x1 x2 ;
actions update
{}
    "{}" sentry 1 $(<)
{}

update x1 ;
update x2 ;
""".format('{', script, '}'))

t.run_build_system()

# workaround for windows error
# 'File x1 not added as expected'
if os.name != "nt":
    t.expect_addition('x1')
    t.expect_addition('x2')
t.expect_output_lines('PARALLEL SLEEP')
t.expect_nothing_more()

t.rm('x1')
t.rm('x2')

# Then test parallel execution suppression by JAM_SEMAPHORE
t.write('file.jam', """\
DEPENDS all : x1 x2 ;
actions update
{}
    "{}" sentry 1 $(<)
{}

JAM_SEMAPHORE on x1 x2 = <s>update_sem ;

update x1 ;
update x2 ;
""".format('{', script, '}'))

expected_output = '''\
...found 3 targets...
...updating 2 targets...
update x1
update x2

...updated 2 targets...
'''

t.run_build_system(stdout=expected_output)

# workaround for windows error
# 'File x1 not added as expected'
if os.name != "nt":
    t.expect_addition('x1')
    t.expect_addition('x2')

t.cleanup()
