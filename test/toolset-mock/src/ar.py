#!/usr/bin/env python3
# coding: utf-8
#
# Copyright 2017-2018 Steven Watanabe
# Copyright 2020 René Ferdinand Rivera Morell
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rcu', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rcu', output_file('libl1.a'), input_file('lib.o'))
command('ar', 'rsc', output_file('libl1.lib'), input_file('lib.obj'))

main()
