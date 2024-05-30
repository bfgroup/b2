/*
Copyright 2024 René Ferdinand Rivera Morell
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#ifndef B2_COMMAND_DB_H
#define B2_COMMAND_DB_H

#include "config.h"

namespace lyra {
class cli;
} // namespace lyra

namespace b2 { namespace command_db {

void declare_args(lyra::cli &);

}} // namespace b2::command_db

#endif
