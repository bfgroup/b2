#!/usr/bin/env python3
#
# Copyright 2017 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

command('libtool', '-static', '-o', output_file('libl1.a'), input_file('lib.o'))
command('libtool', '-static', '-o', output_file('libl1.a'), input_file('lib.o'))

main()
