/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Copyright 2022 René Ferdinand Rivera Morell
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE.txt or
 * https://www.bfgroup.xyz/b2/LICENSE.txt)
 */

/*
 * compile.h - compile parsed jam statements
 */

#ifndef COMPILE_DWA20011022_H
#define COMPILE_DWA20011022_H

#include "config.h"
#include "frames.h"
#include "lists.h"
#include "object.h"
#include "rules.h"

void compile_builtins();

LIST * evaluate_rule(RULE * rule, OBJECT * rulename, FRAME *);
LIST * call_rule(OBJECT * rulename, FRAME * caller_frame, LIST * arg, ...);
LIST * call_rule(OBJECT * rulename, FRAME * caller_frame, LOL * args);
LIST * call_member_rule(OBJECT * rulename,
	FRAME * caller_frame,
	b2::list_ref && self,
	b2::lists && args);

/* Flags for compile_set(), etc */

#define ASSIGN_SET 0x00 /* = assign variable */
#define ASSIGN_APPEND 0x01 /* += append variable */
#define ASSIGN_DEFAULT 0x02 /* set only if unset */

/* Flags for compile_setexec() */

#define EXEC_UPDATED 0x01 /* executes updated */
#define EXEC_TOGETHER 0x02 /* executes together */
#define EXEC_IGNORE 0x04 /* executes ignore */
#define EXEC_QUIETLY 0x08 /* executes quietly */
#define EXEC_PIECEMEAL 0x10 /* executes piecemeal */
#define EXEC_EXISTING 0x20 /* executes existing */

/* Conditions for compile_if() */

#define EXPR_NOT 0 /* ! cond */
#define EXPR_AND 1 /* cond && cond */
#define EXPR_OR 2 /* cond || cond */
#define EXPR_EXISTS 3 /* arg */
#define EXPR_EQUALS 4 /* arg = arg */
#define EXPR_NOTEQ 5 /* arg != arg */
#define EXPR_LESS 6 /* arg < arg  */
#define EXPR_LESSEQ 7 /* arg <= arg */
#define EXPR_MORE 8 /* arg > arg  */
#define EXPR_MOREEQ 9 /* arg >= arg */
#define EXPR_IN 10 /* arg in arg */

#endif

namespace b2 { namespace jam {

template <class... Args>
list_ref run_rule(FRAME * caller_frame, const char * name, Args... arg)
{
	lists args;
	int _[] { 0, ((void)(args |= std::move(arg)), 0)... };
	(void)_;
	return list_ref(call_rule(value_ref(name), caller_frame, args), true);
}

}} // namespace b2::jam
