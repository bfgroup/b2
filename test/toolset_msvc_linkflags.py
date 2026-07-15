#!/usr/bin/env python3
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

# Regression test: <linkflags> passed as part of the "options" parameter of
# a `using msvc : version : command : options ;` toolset-configuration
# statement must reach the linker, per doc/src/overview.adoc's documented,
# cross-toolset contract ("All of B2's standard compiler toolsets accept
# four options cflags, cxxflags, compileflags and linkflags ... specifying
# flags that will be always passed to the corresponding tools").
#
# common.handle-options() (src/tools/common.jam) stores such <linkflags>
# into an OPTIONS variable scoped to "<toolset>.link". gcc.jam's link
# action reads $(OPTIONS), so this works for gcc. msvc.jam's link and
# link.dll actions historically read only $(LINKOPT) $(LINKFLAGS), never
# $(OPTIONS), so a `using msvc : ...`-supplied <linkflags> was silently
# dropped (while a per-target/property <linkflags> requirement, which
# populates LINKFLAGS directly, already worked -- see test/flags.py).
#
# This test uses a private copy of the toolset-mock tree with its own
# project-config.jam and its own mock linker requiring a marker flag on
# every link invocation (both the DLL step and the EXE step). It does
# not modify the shared test/toolset-mock/project-config.jam or mock
# scripts, so it cannot affect toolset_msvc.py's existing coverage.

import BoostBuild
import os
import sys

t = BoostBuild.Tester()

t.set_tree("toolset-mock")

# Replace project-config.jam in this test's own private, per-run copy of
# the tree (Tester.write()/set_tree() never touch the checked-in files)
# with a minimal msvc-only configuration that injects a marker <linkflags>.
t.write("project-config.jam", r"""import modules ;
import os ;

path-constant here : . ;

local PYTHON = [ os.environ PYTHON_CMD ] ;

# Hard-code this so the default /MACHINE:xxx flag is deterministic
# regardless of the CPU architecture of the machine running the test
# suite (see the same workaround, with its own TODO, in the shared
# test/toolset-mock/project-config.jam).
modules.poke .ENVIRON : PROCESSOR_ARCHITEW6432 : amd64 ;

using msvc : 14.3 : :
    <compiler>\"$(PYTHON)\"\ \"$(here)/src/msvc-14.3.py\"
    <linker>\"$(PYTHON)\"\ \"$(here)/src/linkx-linkflags.py\"
    <linkflags>/B2_TEST_LINKFLAGS_MARKER
  ;
""")

# A new mock linker (does not touch the shared src/linkx.py) that requires
# /B2_TEST_LINKFLAGS_MARKER (coming from <linkflags> on the `using`
# statement above) on *every* link invocation: both the DLL step (l1.dll)
# and the EXE step (test.exe). This exercises both msvc.jam's `link` and
# `link.dll` actions. If msvc.jam doesn't forward $(OPTIONS), the marker
# is missing from the real invocation, no registered pattern matches
# (MockProgram requires an exact full-length match), the mock prints
# "ERROR on command: ..." and exits 1, and the build (and this test) fails.
t.write("src/linkx-linkflags.py", r"""from MockProgram import *

if allow_properties("variant=debug", "link=shared", "threading=multi", "runtime-link=shared", "windows-api=desktop"):
    command('link', unordered('/NOLOGO', '/INCREMENTAL:NO', input_file(r'bin\msvc-14.3\debug\threading-multi\lib.obj'), '/DEBUG', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', '/B2_TEST_LINKFLAGS_MARKER', arg('/out:', output_file(r'bin\msvc-14.3\debug\threading-multi\l1.dll')), '/DLL', arg('/IMPLIB:', output_file(r'bin\msvc-14.3\debug\threading-multi\l1.implib'))))
    command('link', unordered('/NOLOGO', '/INCREMENTAL:NO', input_file(r'bin\msvc-14.3\debug\threading-multi\main.obj'), input_file(r'bin\msvc-14.3\debug\threading-multi\l1.implib'), '/DEBUG', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', '/B2_TEST_LINKFLAGS_MARKER', arg('/out:', output_file(r'bin\msvc-14.3\debug\threading-multi\test.exe'))))

main()
""")

os.environ["B2_PROPERTIES"] = "variant=debug link=shared threading=multi runtime-link=shared windows-api=desktop"

t.set_toolset("msvc-14.3", "windows")
# stderr is not checked: msvc.jam's own auto-detection machinery may emit
# unrelated diagnostics (e.g. a missing vswhere.exe) depending on the host
# environment; irrelevant to what this test verifies.
t.run_build_system(["target-os=windows", "--user-config=", "-sPYTHON_CMD=%s" % sys.executable],
                    stderr=None)

t.expect_addition("bin/msvc-14.3/debug/threading-multi/lib.obj")
t.expect_addition("bin/msvc-14.3/debug/threading-multi/l1.implib")
t.expect_addition("bin/msvc-14.3/debug/threading-multi/l1.dll")
t.ignore_addition("bin/msvc-14.3/debug/threading-multi/*l1*.rsp")
t.expect_addition("bin/msvc-14.3/debug/threading-multi/main.obj")
t.expect_addition("bin/msvc-14.3/debug/threading-multi/test.exe")
t.ignore_addition("bin/msvc-14.3/debug/threading-multi/test.rsp")
t.expect_nothing_more()

t.cleanup()
