#!/usr/bin/env python3
#
# Copyright 2017 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

command('clang++', '-print-prog-name=ar', stdout=script('ar.py'))

# all builds are multi-threaded for darwin
if allow_properties("variant=debug", "link=shared", "runtime-link=shared"):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.dylib'), '-single_module', '-dynamiclib', '-install_name', '@rpath/libl1.dylib', input_file('lib.o'), unordered('-g', '-fPIC'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), unordered('-g', '-fPIC'))

if allow_properties("variant=release", "link=shared", "runtime-link=shared"):
    command('clang++', unordered('-O3', '-Wno-inline', '-Wall', '-fPIC'), '-DNDEBUG', '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-v', '-o', output_file('libl1.dylib'), '-single_module', '-dynamiclib', '-install_name', '@rpath/libl1.dylib', input_file('lib.o'), '-fPIC')
    command('clang++', unordered('-O3', '-Wno-inline', '-Wall', '-fPIC'), '-DNDEBUG', '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-v', '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), '-fPIC')

if allow_properties("variant=debug", "link=static", "runtime-link=shared"):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), '-g')

if allow_properties("variant=debug", "link=static", "runtime-link=static"):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), unordered('-g', '-static'))

if allow_properties("variant=debug", "link=shared", "runtime-link=shared", "architecture=x86", "address-model=32"):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-march=i686', '-fPIC', '-m32'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.dylib'), '-single_module', '-dynamiclib', '-install_name', '@rpath/libl1.dylib', input_file('lib.o'), unordered('-g', '-march=i686', '-fPIC', '-m32'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-march=i686', '-fPIC', '-m32'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), unordered('-g', '-march=i686', '-fPIC', '-m32'))

if allow_properties("variant=debug", "link=shared", "runtime-link=shared", "cxxstd=latest"):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC', '-std=c++1z'), '-c', '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.dylib'), '-single_module', '-dynamiclib', '-install_name', '@rpath/libl1.dylib', input_file('lib.o'), unordered('-g', '-fPIC', '-std=c++1z'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC', '-std=c++1z'), '-c', '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), input_file('main.o'), input_file('libl1.dylib'), unordered('-g', '-fPIC', '-std=c++1z'))

main()
