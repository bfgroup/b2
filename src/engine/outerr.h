/*
Copyright 2026 Paolo Pastori
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#ifndef B2_OUTERR_H
#define B2_OUTERR_H

#include "config.h"
#include "jam_fwd.h"

#include <string>

namespace b2 {

/*
 * Error related functions and helpers.
 */

std::string args_to_string(LOL * lol);

void out_emit(const char * prefix, LIST * messages,
    FRAME * frame, bool with_backtrace, bool exit);

void out_error(LIST * messages, FRAME * frame);

} // namespace b2

#endif
