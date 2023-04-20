#!/usr/bin/env python3
#
# Copyright 2017 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

script("libtool.py")

command('g++', '-dumpversion', stdout='4.2.1')

# all builds are multi-threaded for darwin
if allow_properties("variant=debug", "link=shared", "runtime-link=shared"):
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-dynamic', '-gdwarf-2', '-fexceptions', '-fPIC'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', '-dynamiclib', '-Wl,-single_module', '-install_name', 'libl1.dylib', '-o', output_file('libl1.dylib'), input_file('lib.o'), '-headerpad_max_install_names', unordered('-g', '-fPIC'))
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-dynamic', '-gdwarf-2', '-fexceptions', '-fPIC'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), unordered('-g', '-fPIC'))

if allow_properties("variant=release", "link=shared", "runtime-link=shared"):
    command('g++', unordered('-O3', '-Wno-inline', '-Wall', '-dynamic', '-gdwarf-2', '-fexceptions', '-fPIC'), '-DNDEBUG', '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', '-dynamiclib', '-Wl,-single_module', '-install_name', 'libl1.dylib', '-o', output_file('libl1.dylib'), input_file('lib.o'), '-headerpad_max_install_names', unordered(ordered('-Wl,-dead_strip', '-no_dead_strip_inits_and_terms'), '-fPIC'))
    command('g++', unordered('-O3', '-Wno-inline', '-Wall', '-dynamic', '-gdwarf-2', '-fexceptions', '-fPIC'), '-DNDEBUG', '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), unordered(ordered('-Wl,-dead_strip', '-no_dead_strip_inits_and_terms'), '-fPIC'))

if allow_properties("variant=debug", "link=static", "runtime-link=shared"):
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-gdwarf-2', '-fexceptions'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-gdwarf-2', '-fexceptions'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), '-g')

if allow_properties("variant=debug", "link=static", "runtime-link=static"):
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-gdwarf-2', '-fexceptions'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-gdwarf-2', '-fexceptions'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), unordered('-g', ordered('-nodefaultlibs', '-shared-libgcc', '-lstdc++-static', '-lgcc_eh', '-lgcc', '-lSystem'), '-static'))

main()
