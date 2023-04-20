#!/usr/bin/env python3
#
# Copyright 2018 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

if allow_properties('variant=debug', 'link=shared', 'threading=single', 'runtime-link=shared'):
    command('ld', '-o', output_file('libl1.so'), input_file('lib.o'), unordered('-g', '-fPIC'), '-fpic', '-shared', '-non-static')
    command('ld', '-o', output_file('test'), input_file('main.o'), input_file('libl1.so'), unordered('-g', '-fPIC'))

if allow_properties('variant=release', 'link=shared', 'threading=single', 'runtime-link=shared', 'strip=on'):
    command('ld', '-t', '-o', output_file('libl1.so'), input_file('lib.o'), unordered('-fPIC', '-Wl,--strip-all'), '-fpic', '-shared', '-non-static')
    command('ld', '-t', '-o', output_file('test'), input_file('main.o'), input_file('libl1.so'), unordered('-fPIC', '-Wl,--strip-all'))

if allow_properties('variant=debug', 'link=shared', 'threading=multi', 'runtime-link=shared'):
    command('ld', '-o', output_file('libl1.so'), input_file('lib.o'), unordered('-g', '-fPIC'), '-fpic', '-shared', '-non-static')
    command('ld', '-o', output_file('test'), input_file('main.o'), input_file('libl1.so'), unordered('-g', '-fPIC'))

if allow_properties('variant=debug', 'link=static', 'threading=single', 'runtime-link=shared'):
    command('ld', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), '-g')

if allow_properties('variant=debug', 'link=static', 'threading=single', 'runtime-link=static'):
    command('ld', '-o', output_file('test'), input_file('main.o'), input_file('libl1.a'), unordered('-g'))

if allow_properties('variant=debug', 'link=shared', 'threading=single', 'runtime-link=shared', 'architecture=x86', 'address-model=32'):
    command('ld', '-o', output_file('libl1.so'), input_file('lib.o'), unordered('-g', '-march=i686', '-fPIC', '-m32'), '-fpic', '-shared', '-non-static')
    command('ld', '-o', output_file('test'), input_file('main.o'), input_file('libl1.so'), unordered('-g', '-march=i686', '-fPIC', '-m32'))

main()
