#!/usr/bin/env python3
#
# Copyright 2017 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

command('g++', '-print-prog-name=ar', stdout=script('ar.py'))

# all builds are multi-threaded for darwin
if allow_properties("variant=debug", "link=shared", "runtime-link=shared"):
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', '-o', output_file('libl1.dylib'), '-shared', input_file('lib.o'), unordered('-g', '-fPIC'))
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-Wl,-rpath', arg('-Wl,', target_path('libl1.dylib')), '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), unordered('-g', '-fPIC'))

if allow_properties("variant=release", "link=shared", "runtime-link=shared"):
    command('g++', unordered('-O3', '-finline-functions', '-Wno-inline', '-Wall', '-fPIC', '-DNDEBUG'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', '-o', output_file('libl1.dylib'), '-shared', input_file('lib.o'), '-fPIC')
    command('g++', unordered('-O3', '-finline-functions', '-Wno-inline', '-Wall', '-fPIC', '-DNDEBUG'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-Wl,-rpath', arg('-Wl,', target_path('libl1.dylib')), '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), '-fPIC')

if allow_properties("variant=debug", "link=static", "runtime-link=shared"):
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), '-g')

if allow_properties("variant=debug", "link=static", "runtime-link=static"):
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('g++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('g++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), unordered('-g', '-static'))

main()
