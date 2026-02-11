#!/usr/bin/env python3

# Copyright 2006 Rene Rivera.
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)

import BoostBuild

t = BoostBuild.Tester(["-d1"], pass_toolset=0)

t.write("sleep.bat", """\
::@timeout /T %1 /NOBREAK >nul
@ping 127.0.0.1 -n 2 -w 1000 >nul
@ping 127.0.0.1 -n %1 -w 1000 >nul
@exit /B 0
""")

t.write("file.jam", """\
if $(NT)
{
    actions sleeper
    {
        call sleep.bat $(<:B)
    }
}
else
{
    actions sleeper
    {
        sleep $(<:B)
    }
}

rule sleeper
{
    DEPENDS $(<) : $(>) ;
}

NOTFILE front ;
sleeper 1.a : front ;
sleeper 2.a : front ;
sleeper 3.a : front ;
sleeper 4.a : front ;
NOTFILE choke ;
DEPENDS choke : 1.a 2.a 3.a 4.a ;
sleeper 1.b : choke ;
sleeper 2.b : choke ;
sleeper 3.b : choke ;
sleeper 4.b : choke ;
DEPENDS bottom : 1.b 2.b 3.b 4.b ;
DEPENDS all : bottom ;
""")

t.run_build_system(["-ffile.jam", "-j4"])
t.expect_output_lines("""\
...found 12 targets...
...updating 8 targets...
sleeper [1-4].a
sleeper [1-4].a
sleeper [1-4].a
sleeper [1-4].a
sleeper [1-4].b
sleeper [1-4].b
sleeper [1-4].b
sleeper [1-4].b

...updated 8 targets...
""")

t.cleanup()
