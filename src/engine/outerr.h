/*
Copyright 2026 Paolo Pastori
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#ifndef B2_OUTERR_H
#define B2_OUTERR_H

#include <string>
#include <vector>

typedef struct _lol LOL;
typedef struct frame FRAME;

namespace b2 {

/*
 * Error related functions and helpers.
 */

std::string args_to_string(LOL * lol);

using message_list_t = std::vector<std::string>;

void out_emit(const std::string & prefix, const message_list_t & messages,
    FRAME * frame, bool with_backtrace, bool exit);

void out_error(const message_list_t & messages, FRAME * frame);

} // namespace b2

#endif
