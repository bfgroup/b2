#!/usr/bin/env python3

# Copyright 2026 Paolo Pastori
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)

"""
Test the debugger console interface, either with the linenoise
library support (Linux, MacOS, *BSD), or without (Windows.)
"""

import BoostBuild
import TestCmd
import os
import re

def split_stdin_stdout(text):
    # stdin is all text after the prompt up to and including
    # the next newline.  Everything else is stdout.  stdout
    # may contain regular expressions enclosed in {{}}.
    pattern = re.compile(r'(?<=\(b2db\)\n)((?:.*)\n)')
    stdin = ''.join(re.findall(pattern, text))
    stdout = re.sub(pattern, '', text)
    outside_pattern = re.compile(r'(?:\A|(?<=\}\}))(?:[^{]|(?:\{(?!\{)))*(?:(?=\{\{)|\Z)')

    def escape_line(line):
        line = re.sub(outside_pattern, lambda m: m.group(0), line)
        return re.sub(r'\{\{|\}\}', '', line)

    # On Windows the debugger do not make use of linenoise library and
    # always emit a prompt, while linenoise does for interactive input
    # only, i.e. when input comes from a tty (not from a file or pipe.)
    win = os.name == "nt"
    stdout = '\n'.join([escape_line(line)
        for line in stdout.split('\n') if win or line != '(b2db)'])
    return (stdin, stdout)

def run(tester, io):
    (input, output) = split_stdin_stdout(io)
    tester.run_build_system(stdin=input, stdout=output, match=TestCmd.match_re)

def make_tester():
    return BoostBuild.Tester(["-dconsole"], pass_toolset=False)

def test_exec_run():
    t = make_tester()
    t.write("test.jam", """\
UPDATE ;
    """)
    run(t, """\
Starting program: {} -ftest.jam
Child {{{{[0-9]+}}}} exited with status 0
(b2db)
run -ftest.jam
""".format(t.program[0]))
    t.cleanup()


def test_exit_status():
    t = make_tester()
    t.write("test.jam", """\
EXIT : 1 ;
    """)
    run(t, """\
Starting program: {} -ftest.jam

Child {{{{[0-9]+}}}} exited with status 1
(b2db)
run -ftest.jam
""".format(t.program[0]))
    t.cleanup()

def test_exec_step():
    t = make_tester()
    t.write("test.jam", """\
rule g ( )
{
    a = 1 ;
    b = 2 ;
}
rule f ( )
{
    g ;
    c = 3 ;
}
f ;
""")
    run(t, """\
Breakpoint 1 set at f
(b2db)
break f
Starting program: {} -ftest.jam
Breakpoint 1, f \\( \\) at test.jam:8
(b2db)
run -ftest.jam
8	    g ;
(b2db)
step
3	    a = 1 ;
(b2db)
step
4	    b = 2 ;
(b2db)
step
9	    c = 3 ;
don't know how to make all
Child {{{{[0-9]+}}}} exited with status 1
(b2db)
step
""".format(t.program[0]))
    t.cleanup()

def test_exec_next():
    t = make_tester()
    t.write("test.jam", """\
rule g ( )
{
    a = 1 ;
}
rule f ( )
{
    g ;
    b = 2 ;
    c = 3 ;
}
rule h ( )
{
    f ;
    g ;
}
h ;
d = 4 ;
""")
    run(t, """\
Breakpoint 1 set at f
(b2db)
break f
Starting program: {} -ftest.jam
Breakpoint 1, f \\( \\) at test.jam:7
(b2db)
run -ftest.jam
7	    g ;
(b2db)
next
8	    b = 2 ;
(b2db)
next
9	    c = 3 ;
(b2db)
next
14	    g ;
(b2db)
next
17	d = 4 ;
(b2db)
quit
""".format(t.program[0]))
    t.cleanup()

def test_exec_finish():
    t = make_tester()
    t.write("test.jam", """\
rule f ( )
{
    a = 1 ;
}
rule g ( )
{
    f ;
    b = 2 ;
    i ;
}
rule h ( )
{
    g ;
    i ;
}
rule i ( )
{
    c = 3 ;
}
h ;
d = 4 ;
""")
    run(t, """\
Breakpoint 1 set at f
(b2db)
break f
Starting program: {} -ftest.jam
Breakpoint 1, f \\( \\) at test.jam:3
(b2db)
run -ftest.jam
3	    a = 1 ;
(b2db)
finish
8	    b = 2 ;
(b2db)
finish
14	    i ;
(b2db)
finish
21	d = 4 ;
(b2db)
quit
""".format(t.program[0]))
    t.cleanup()

def test_breakpoints():
    """Tests the interaction between the following commands:
    break, clear, delete, disable, enable"""
    t = make_tester()
    t.write("test.jam", """\
rule f ( )
{
    a = 1 ;
}
rule g ( )
{
    b = 2 ;
}
rule h ( )
{
    c = 3 ;
    d = 4 ;
}
f ;
g ;
h ;
UPDATE ;
""")
    run(t, """\
Breakpoint 1 set at f
(b2db)
break f
Starting program: {0} -ftest.jam
Breakpoint 1, f \\( \\) at test.jam:3
(b2db)
run -ftest.jam
3	    a = 1 ;
(b2db)
kill
Breakpoint 2 set at g
(b2db)
break g
(b2db)
disable 1
Starting program: {0} -ftest.jam
Breakpoint 2, g \\( \\) at test.jam:7
(b2db)
run -ftest.jam
7	    b = 2 ;
(b2db)
kill
(b2db)
enable 1
Starting program: {0} -ftest.jam
Breakpoint 1, f \\( \\) at test.jam:3
(b2db)
run -ftest.jam
3	    a = 1 ;
(b2db)
kill
(b2db)
delete 1
Starting program: {0} -ftest.jam
Breakpoint 2, g \\( \\) at test.jam:7
(b2db)
run -ftest.jam
7	    b = 2 ;
(b2db)
quit
""".format(t.program[0]))
    t.cleanup()


test_exec_run()
test_exit_status()
test_exec_step()
test_exec_next()
test_exec_finish()
test_breakpoints()
