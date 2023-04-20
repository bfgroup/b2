# Copyright 2022 Nikita Kniazev
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# https://www.bfgroup.xyz/b2/LICENSE.txt)

from MockProgram import *

if allow_properties("variant=debug", "link=shared", "threading=multi", "runtime-link=shared", "windows-api=desktop"):
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('lib.obj'), '/DEBUG', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('l1.dll')), '/DLL', arg('/IMPLIB:', output_file('l1.implib')))
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('main.obj'), input_file('l1.implib'), '/DEBUG', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('test.exe')))

if allow_properties("variant=release", "link=shared", "threading=multi", "runtime-link=shared", "windows-api=desktop"):
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('lib.obj'), '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('l1.dll')), '/DLL', arg('/IMPLIB:', output_file('l1.implib')))
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('main.obj'), input_file('l1.implib'), '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('test.exe')))

if allow_properties("variant=debug", "link=static", "threading=multi", "runtime-link=shared", "windows-api=desktop"):
    command('link', '/lib', '/NOLOGO', arg('/out:', output_file('l1.lib')), input_file('lib.obj'))
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('main.obj'), input_file('l1.lib'), '/DEBUG', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('test.exe')))

if allow_properties("variant=debug", "link=static", "threading=single", "runtime-link=static", "windows-api=desktop"):
    command('link', '/lib', '/NOLOGO', arg('/out:', output_file('l1.lib')), input_file('lib.obj'))
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('main.obj'), input_file('l1.lib'), '/DEBUG', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('test.exe')))

if allow_properties("variant=debug", "link=shared", "threading=multi", "runtime-link=shared", "windows-api=store"):
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('lib.obj'), '/DEBUG', '/APPCONTAINER', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('l1.dll')), '/DLL', arg('/IMPLIB:', output_file('l1.implib')))
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('main.obj'), input_file('l1.implib'), '/DEBUG', '/APPCONTAINER', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('test.exe')))

if allow_properties("variant=debug", "link=shared", "threading=multi", "runtime-link=shared", "windows-api=phone"):
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('lib.obj'), '/DEBUG', '/APPCONTAINER', '/NODEFAULTLIB:kernel32.lib', '/NODEFAULTLIB:ole32.lib', 'WindowsPhoneCore.lib', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('l1.dll')), '/DLL', arg('/IMPLIB:', output_file('l1.implib')))
    command('link', '/NOLOGO', '/INCREMENTAL:NO', input_file('main.obj'), input_file('l1.implib'), '/DEBUG', '/APPCONTAINER', '/NODEFAULTLIB:kernel32.lib', '/NODEFAULTLIB:ole32.lib', 'WindowsPhoneCore.lib', '/MACHINE:X64', '/MANIFEST:EMBED', '/subsystem:console', arg('/out:', output_file('test.exe')))

main()
