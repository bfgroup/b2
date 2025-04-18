/*
 * Copyright 2015 Steven Watanabe
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE.txt or copy at
 * https://www.bfgroup.xyz/b2/LICENSE.txt)
 */

#ifndef DEBUGGER_SW20150314_H
#define DEBUGGER_SW20150314_H

#include "config.h"
#include <setjmp.h>
#include "object.h"
#include "frames.h"
#include "lists.h"

#ifdef JAM_DEBUGGER

void debug_on_instruction( FRAME * frame, OBJECT * file, int line );
void debug_on_enter_function( FRAME * frame, OBJECT * name, OBJECT * file, int line );
void debug_on_exit_function( OBJECT * name );
int debugger( void );
void debugger_done();

struct debug_child_data_t
{
    int argc;
    const char * * argv;
    jmp_buf jmp;
};

extern struct debug_child_data_t debug_child_data;
extern b2::list_ref debug_print_result;
extern const char debugger_opt[];

#define DEBUG_INTERFACE_CONSOLE (global_config::debug_interface_console)
#define DEBUG_INTERFACE_MI (global_config::debug_interface_mi)
#define DEBUG_INTERFACE_CHILD (global_config::debug_interface_child)

#define debug_is_debugging() ( globs.debug_interface != global_config::debug_interface_no )
#define debug_on_enter_function( frame, name, file, line )      \
    ( debug_is_debugging()?                                     \
        debug_on_enter_function( frame, name, file, line ) :    \
        (void)0 )
#define debug_on_exit_function( name )                          \
    ( debug_is_debugging()?                                     \
        debug_on_exit_function( name ) :                        \
        (void)0 )

#if NT

void debug_init_handles( const char * in, const char * out );

#endif

#else

#define debug_on_instruction( frame, file, line ) ( ( void )0 )
#define debug_on_enter_function( frame, name, file, line ) ( ( void )0 )
#define debug_on_exit_function( name ) ( ( void )0 )
#define debug_is_debugging() ( 0 )

#endif

#endif
