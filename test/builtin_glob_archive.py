#!/usr/bin/env python3

# Copyright 2014 Steven Watanabe
# Copyright 2015 Artur Shepilko
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)

# This tests the GLOB_ARCHIVE rule.

import os
import sys

import BoostBuild

vms = ( os.name == 'posix' and sys.platform == 'OpenVMS')

t = BoostBuild.Tester()

## Setup test archive sources and symbols they contain.
sources = {
    "a.cpp" : ["a"],
    "b.cpp" : ["b"],
    "b_match.cpp" : ["b_match"],
    "c/nopath_check.cpp" : ["nopath_check"],
    "CaseCheck.cpp" : ["CaseCheck"],
    "seq_check1.cpp" : ["seq_check1"],
    "seq_check2.cpp" : ["seq_check2"],
    "seq_check3.cpp" : ["seq_check3"],
    "symbols_check.c" : ["symbol", "symbol_match"],
    "members_and_symbols_check.c" : ["member_and_symbol_match"],
    "symbol_case_check.c" : ["SymbolCaseCheck"],
    "main_check.cpp" : ["main"]
}


def create_sources(path, sources):
    for s in sources :
        f = os.path.join(path, s)
        t.write(f,
                "".join(["int {}() {{ return 0; }}\n".format(sym)
                         for sym in sources[s]]))


def setup_archive(name, sources):
    archive = t.adjust_names(name)[0]
    obj_suffix = t.adjust_names(".obj")[0]
    t.write("jamroot.jam","")
    ## sort the sources, so we can test order of the globbed members
    jl = [ "static-lib {} :".format(name.split(".")[0]) ]
    jl.extend(sorted(sources))
    jl.append(";\n")
    t.write("lib/jamfile.jam", "\n".join(jl))
    create_sources("lib", sources)
    t.run_build_system(subdir="lib")
    built_archive = "lib/bin/$toolset/debug*/" + name
    t.expect_addition(built_archive)
    t.copy(built_archive, name)
    t.rm("lib")
    return archive, obj_suffix


def list_archive(archive):
    t.write("file.jam", """\
ECHO [ SHELL "ar -t {} 2>&1" ] ;
UPDATE ;
""".format(archive))
    t.run_build_system(["-ffile.jam"], stdout="")
    t.rm("file.jam")

def list_dir(path):
    t.write("file.jam", """\
ECHO [ SHELL "ls {} 2>&1" ] ;
UPDATE ;
""".format(path))
    t.run_build_system(["-ffile.jam"], stdout="")
    t.rm("file.jam")

def find(path, target):
    t.write("file.jam", """\
ECHO [ SHELL "find {} -name {} 2>&1" ] ;
UPDATE ;
""".format(path, target))
    t.run_build_system(["-ffile.jam"], stdout="")
    t.rm("file.jam")

def cat_file(path):
    t.write("file.jam", """\
ECHO [ SHELL "cat {} 2>&1" ] ;
UPDATE ;
""".format(path))
    t.run_build_system(["-ffile.jam"], stdout="")
    t.rm("file.jam")

def test_glob_archive(archives, glob, expected, sort_results = False):
    ## replace placeholders
    glob = glob.replace("$archive1", archives[0]).replace("$obj", obj_suffix)
    expected = [ m.replace("$archive1",
               archives[0]).replace("$obj", obj_suffix) for m in expected ]
    if len(archives) > 1 :
        glob = glob.replace("$archive2", archives[1]).replace("$obj", obj_suffix)
        expected = [ m.replace("$archive2",
               archives[1]).replace("$obj", obj_suffix) for m in expected ]
    ## create test jamfile
    if sort_results : glob = "[ SORT {} ]".format(glob)
    t.write("file.jam", """\
for local p in {}
{{
    ECHO $(p) ;
}}
UPDATE ;
""".format(glob))
    ## run test jamfile and match against expected results
    if sort_results : expected.sort()
    t.run_build_system(["-ffile.jam", "-d+6", "-d+14"], stdout="\n".join(expected + [""]))
    t.rm("file.jam")


## RUN TESTS
#archive1, obj_suffix = setup_archive("auxilliary1.lib", sources)

## list archive contents
#list_archive(archive1)

## match exact
#test_glob_archive([archive1], "[ GLOB_ARCHIVE $archive1 : a$obj ]",
#                  ["$archive1(a$obj)"])

#list_dir("/")
#find("/Library", "ar.h")
#cat_file("/Library/Developer/CommandLineTools/SDKs/MacOSX15.5.sdk/usr/include/ar.h")
find("/Library", "_bounds.h")

t.cleanup()

