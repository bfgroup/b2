/*
Copyright 2026 Paolo Pastori
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#ifndef B2_JAM_FWD_H
#define B2_JAM_FWD_H

/*
 * forward declarations,
 * to avoid header inclusion.
 */

// frames.h
typedef struct frame FRAME;

// function.h
typedef struct _function FUNCTION;
typedef FUNCTION * function_ptr;

// jam_strings.h
typedef struct string string;

// lists.h
struct LIST;
typedef LIST * list_ptr;
typedef struct _lol LOL;

// modules.h
typedef struct module_t module_t;
typedef module_t * module_ptr;

// object.h, see below

// parse.h
typedef struct _PARSE PARSE;

// pathsys.h
typedef struct _pathname PATHNAME;

// regexp.h
namespace b2 { namespace regex {
struct program;
}}

// rules.h
typedef struct _rule RULE;
typedef struct _target TARGET;

// timestamp.h
typedef struct timestamp timestamp;

// value.h
namespace b2 {
struct value;
typedef value * value_ptr;
}

// object.h
typedef b2::value OBJECT;

#endif
