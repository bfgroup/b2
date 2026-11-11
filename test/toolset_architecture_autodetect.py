#!/usr/bin/env python3
#
# Distributed under the Boost Software License, Version 1.0.
# https://www.bfgroup.xyz/b2/LICENSE.txt

# Test for autodetecting <architecture> from the compiler predefined macros
# when it is not given explicitly via command line or config file.
# This validation covers gcc, clang-linux and clang-darwin toolsets.
# The default is only set when the compiler targets an
# architecture other than the one b2 runs on, so native builds are unchanged.

import BoostBuild
import platform
import sys
import textwrap

# Compiler macro to report, and the <architecture> value it maps to.
X86 = ("__x86_64__", "x86")
ARM = ("__aarch64__", "arm")
POWER = ("__powerpc64__", "power")

MOCK_COMPILER_TEMPLATE = '''#!/usr/bin/env python3
import sys

args = sys.argv[1:]

if args == ["-dumpmachine"]:
    print("unknown")
    sys.exit(0)

if args == ["-print-prog-name=ar"]:
    print("ar")
    sys.exit(0)

if "-dM" in args:
    if args.count("-arch") > 1:
        sys.stderr.write("mock: multiple -arch with -E\\n")
        sys.exit(1)
    print("#define __GNUC__ 12")
    print("#define @MACRO@ 1")
    sys.exit(0)

if "-c" in args and "-o" in args:
    with open(args[args.index("-o") + 1], "w") as f:
        f.write("mock object\\n")
    sys.exit(0)

sys.exit(1)
'''


def host_architecture():
    machine = platform.machine().lower()
    if machine in ("x86_64", "amd64", "i386", "i686", "x86"):
        return X86
    if machine in ("arm64", "aarch64"):
        return ARM
    return None


def cross_architecture():
    """An architecture that is not the one b2 is running on."""
    return POWER if host_architecture() in (X86, ARM) else X86


def expect_no_architecture(t):
    """Test that no <architecture> ended up in the build path.

       Used together with a wildcard in the expected path, as b2 may add other
       properties to it depending on the host, like threadapi on Windows.
    """
    if "architecture-" in t.stdout():
        BoostBuild.annotation("failure", "architecture was defaulted")
        t.fail_test(1)


def build(toolset, macro, using_options="", extra_args=None):
    t = BoostBuild.Tester(pass_toolset=0)
    t.write("fake-compiler.py", MOCK_COMPILER_TEMPLATE
            .replace("@MACRO@", macro))
    t.write("project-config.jam", textwrap.dedent('''
        import os ;
        path-constant HERE : . ;
        local PYTHON = [ os.environ PYTHON_CMD ] ;
        using %s : autodetect : $(PYTHON) $(HERE)/fake-compiler.py : %s ;
        ''' % (toolset, using_options)))
    t.write("Jamroot.jam", "obj test : test.cpp ;")
    t.write("test.cpp", "int f() { return 0; }")
    t.run_build_system(["-sPYTHON_CMD=" + sys.executable,
                        "toolset=%s-autodetect" % toolset, "test"]
                       + (extra_args or []))
    return t


def test_cross_architecture_autodetect(toolset):
    """Test that a cross architecture is correctly autodetected.
    """
    macro, arch = cross_architecture()
    t = build(toolset, macro)
    t.expect_addition("bin/%s-autodetect/debug/architecture-%s*/test.o*"
                      % (toolset, arch))
    t.cleanup()


def test_native_architecture_not_defaulted(toolset):
    """Test that the native architecture is not defaulted.
    """
    host = host_architecture()
    if not host:
        return
    # Nothing is added to the build path for a native build.
    t = build(toolset, host[0])
    t.expect_addition("bin/%s-autodetect/debug*/test.o*" % toolset)
    expect_no_architecture(t)
    t.cleanup()


def test_explicit_architecture_wins(toolset):
    """Test that an explicit architecture setting overrides autodetection.
    """
    macro, _ = cross_architecture()
    t = build(toolset, macro, extra_args=["architecture=mips"])
    t.expect_addition("bin/%s-autodetect/debug/architecture-mips*/test.o*"
                      % toolset)
    t.cleanup()


def test_universal_build_not_probed(toolset):
    """Test that a universal build is not probed.

       The mock compiler fails when preprocessing with several -arch options,
       which must therefore not be attempted. No default, and no error.
    """
    macro, _ = cross_architecture()
    t = build(toolset, macro,
              using_options='<cxxflags>"-arch a" <cxxflags>"-arch b"')
    t.expect_addition("bin/%s-autodetect/debug*/test.o*" % toolset)
    expect_no_architecture(t)
    t.cleanup()


def test_stderr_of_probe_is_silenced(toolset):
    """Test that the stderr of the probe is silenced.

       No output from the probe should appear on the terminal.
    """
    macro, _ = cross_architecture()
    t = BoostBuild.Tester(pass_toolset=0)
    t.write("fake-compiler.py", MOCK_COMPILER_TEMPLATE
            .replace("@MACRO@", macro)
            .replace('if "-dM" in args:',
                     'if "-dM" in args:\n    sys.stderr.write("mock-noise\\n")'))
    t.write("project-config.jam", textwrap.dedent('''
        import os ;
        path-constant HERE : . ;
        local PYTHON = [ os.environ PYTHON_CMD ] ;
        using %s : autodetect : $(PYTHON) $(HERE)/fake-compiler.py ;
        ''' % toolset))
    t.write("Jamroot.jam", "obj test : test.cpp ;")
    t.write("test.cpp", "int f() { return 0; }")
    t.run_build_system(["-sPYTHON_CMD=" + sys.executable,
                        "toolset=%s-autodetect" % toolset, "test"])
    t.expect_addition("bin/%s-autodetect/debug/architecture-%s*/test.o*"
                      % (toolset, cross_architecture()[1]))
    if "mock-noise" in t.stdout() + t.stderr():
        BoostBuild.annotation("failure", "probe stderr leaked")
        t.fail_test(1)
    t.cleanup()


for toolset in ("gcc", "clang-linux", "clang-darwin"):
    test_cross_architecture_autodetect(toolset)
    test_native_architecture_not_defaulted(toolset)
    test_explicit_architecture_wins(toolset)
    test_universal_build_not_probed(toolset)
    test_stderr_of_probe_is_silenced(toolset)
