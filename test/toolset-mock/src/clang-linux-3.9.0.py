#!/usr/bin/env python3
# coding: utf-8
#
# Copyright 2017 Steven Watanabe
# Copyright 2020 René Ferdinand Rivera Morell
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

command('clang++', '-print-prog-name=ar', stdout=script('ar.py'))

# target-os=linux ..

if allow_properties('target-os=linux', 'variant=debug', 'link=shared', 'threading=single', 'runtime-link=shared'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC', '-c'), '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.so'), '-Wl,-soname', '-Wl,libl1.so', '-shared', '-Wl,--start-group', input_file('lib.o'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g', '-fPIC'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-fPIC', '-c'), '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', unordered(ordered('-o', output_file('test')), ordered('-Wl,-rpath', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,-rpath-link', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,--start-group', input_file('main.o'), input_file('libl1.so'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group'), unordered('-g', '-fPIC')))

if allow_properties('target-os=linux', 'variant=release', 'link=shared', 'threading=single', 'runtime-link=shared', 'strip=on'):
    command('clang++', unordered('-O3', '-Wno-inline', '-Wall', '-fPIC', '-DNDEBUG', '-c'), '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.so'), '-Wl,-soname', '-Wl,libl1.so', '-shared', '-Wl,--start-group', input_file('lib.o'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-fPIC', '-Wl,--strip-all'))
    command('clang++', unordered('-O3', '-Wno-inline', '-Wall', '-fPIC', '-DNDEBUG', '-c'), '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', unordered(ordered('-o', output_file('test')), ordered('-Wl,-rpath', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,-rpath-link', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,--start-group', input_file('main.o'), input_file('libl1.so'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group'), unordered('-fPIC', '-Wl,--strip-all')))

if allow_properties('target-os=linux', 'variant=debug', 'link=shared', 'threading=multi', 'runtime-link=shared'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-pthread', '-fPIC', '-c'), '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.so'), '-Wl,-soname', '-Wl,libl1.so', '-shared', '-Wl,--start-group', input_file('lib.o'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-lrt', '-Wl,--end-group', unordered('-g', '-pthread', '-fPIC'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-pthread', '-fPIC', '-c'), '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', unordered(ordered('-o', output_file('test')), ordered('-Wl,-rpath', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,-rpath-link', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,--start-group', input_file('main.o'), input_file('libl1.so'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-lrt', '-Wl,--end-group'), unordered('-g', '-pthread', '-fPIC')))

if allow_properties('target-os=linux', 'variant=debug', 'link=static', 'threading=single', 'runtime-link=shared'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), '-Wl,--start-group', input_file('main.o'), input_file('libl1.a'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', '-g')

if allow_properties('target-os=linux', 'variant=debug', 'link=static', 'threading=single', 'runtime-link=static'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), '-Wl,--start-group', input_file('main.o'), input_file('libl1.a'), '-Wl,--end-group', unordered('-g', '-static'))

if allow_properties('target-os=linux', 'variant=debug', 'link=shared', 'threading=single', 'runtime-link=shared', 'architecture=x86', 'address-model=32'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-march=i686', '-m32', '-fPIC', '-c', '--target=i386-pc-linux'), '-o', output_file('lib.o'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('libl1.so'), '-Wl,-soname', '-Wl,libl1.so', '-shared', '-Wl,--start-group', input_file('lib.o'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g', '-march=i686', '-fPIC', '-m32', '--target=i386-pc-linux'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-march=i686', '-m32', '-fPIC', '-c', '--target=i386-pc-linux'), '-o', output_file('main.o'), input_file(source='main.cpp'))
    command('clang++', unordered(ordered('-o', output_file('test')), ordered('-Wl,-rpath', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,-rpath-link', arg('-Wl,', target_path('libl1.so'))), ordered('-Wl,--start-group', input_file('main.o'), input_file('libl1.so'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group'), unordered('-g', '-march=i686', '-fPIC', '-m32', '--target=i386-pc-linux')))

# target-os=windows ..

if allow_properties('target-os=windows', 'variant=debug', 'link=shared', 'threading=single', 'runtime-link=shared'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('lib.obj'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('l1.dll'), '-shared', '-Wl,--start-group', input_file('lib.obj'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('main.obj'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test.exe'), '-Wl,--start-group', input_file('main.obj'), input_file('l1.dll'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g'))

if allow_properties('target-os=windows', 'variant=release', 'link=shared', 'threading=single', 'runtime-link=shared', 'strip=on'):
    command('clang++', unordered('-O3', '-Wno-inline', '-Wall', '-DNDEBUG', '-c'), '-o', output_file('lib.obj'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('l1.dll'), '-shared', '-Wl,--start-group', input_file('lib.obj'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-Wl,--strip-all'))
    command('clang++', unordered('-O3', '-Wno-inline', '-Wall', '-DNDEBUG', '-c'), '-o', output_file('main.obj'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), '-Wl,--start-group', input_file('main.obj'), input_file('l1.dll'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-Wl,--strip-all'))

if allow_properties('target-os=windows', 'variant=debug', 'link=shared', 'threading=multi', 'runtime-link=shared'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-pthread', '-c'), '-o', output_file('lib.obj'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('l1.dll'), '-shared', '-Wl,--start-group', input_file('lib.obj'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g', '-pthread'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-pthread', '-c'), '-o', output_file('main.obj'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test'), '-Wl,--start-group', input_file('main.obj'), input_file('l1.dll'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g', '-pthread'))

if allow_properties('target-os=windows', 'variant=debug', 'link=static', 'threading=single', 'runtime-link=shared'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('lib.obj'), input_file(source='lib.cpp'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('main.obj'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test.exe'), '-Wl,--start-group', input_file('main.obj'), input_file('libl1.lib'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', '-g')

if allow_properties('target-os=windows', 'variant=debug', 'link=static', 'threading=single', 'runtime-link=static'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('lib.obj'), input_file(source='lib.cpp'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-c'), '-o', output_file('main.obj'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test.exe'), '-Wl,--start-group', input_file('main.obj'), input_file('libl1.lib'), '-Wl,--end-group', unordered('-g', '-static'))

if allow_properties('target-os=windows', 'variant=debug', 'link=shared', 'threading=single', 'runtime-link=shared', 'architecture=x86', 'address-model=32'):
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-march=i686', '-m32', '-c'), '-o', output_file('lib.obj'), input_file(source='lib.cpp'))
    command('clang++', '-o', output_file('l1.dll'), '-shared', '-Wl,--start-group', input_file('lib.obj'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g', '-march=i686', '-m32'))
    command('clang++', unordered('-O0', '-fno-inline', '-Wall', '-g', '-march=i686', '-m32', '-c'), '-o', output_file('main.obj'), input_file(source='main.cpp'))
    command('clang++', '-o', output_file('test.exe'), '-Wl,--start-group', input_file('main.obj'), input_file('l1.dll'), '-Wl,-Bstatic', '-Wl,-Bdynamic', '-Wl,--end-group', unordered('-g', '-march=i686', '-m32'))

main()
