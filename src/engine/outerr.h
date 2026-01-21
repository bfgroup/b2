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

void out_error(const std::string & prefix,
    const std::vector<std::string> & messages, FRAME * frame);

} // namespace b2

#endif
