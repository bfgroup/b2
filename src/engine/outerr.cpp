/*
Copyright 2026 Paolo Pastori
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#include "jam.h"
#include "outerr.h"

#include "lists.h"
#include "output.h"
#include "startup.h"


std::string b2::args_to_string(LOL * lol)
{
    std::string res;
    for (int32_t i = 0; i < lol->count; ++i)
    {
        if (i) res += " :";
        for ( auto el : b2::list_cref(lol->list[i]) )
            res += std::string(" ") + el->str();
    }
    return res;
}

// TODO: move these from builtins
//void print_source_line( FRAME * );
void backtrace_line   ( FRAME * );
void backtrace        ( FRAME * );

/*
 * Error function that try to mimic the b2::jam::errors::backtrace.
 */
void b2::out_emit(const char * prefix,
    LIST * messages, FRAME * frame,
    bool with_backtrace, bool exit)
{
    bool has_prev = false;
    if (frame)
    {
        has_prev = frame->prev != nullptr;
        backtrace_line( has_prev ? frame->prev : frame );
    }

    auto print_ln = [prefix](OBJECT * ln) {
        ::b2::value::str_view ln_view = ln->as_string();
        if (ln_view.size == 0) return;
        if (prefix) out_printf( "%s ", prefix );
        out_printf( "%s\n", ln_view.str );
    };
    for (OBJECT * msg : list_cref(messages)) print_ln(msg);

    if (with_backtrace && has_prev) backtrace( frame->prev );

    if (exit) b2::clean_exit( EXITBAD );
}

void b2::out_error(LIST * messages, FRAME * frame)
{
    out_emit("error:", messages, frame, true, true);
}
