/*
Copyright 2024 René Ferdinand Rivera Morell
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#ifndef B2_MOD_ARGS_H
#define B2_MOD_ARGS_H

#include "bind.h"
#include "lists.h"
#include "value.h"

/* tag::reference[]

[[b2.reference.modules.args]]
= `args` module.

end::reference[] */

namespace b2 { namespace args {

/* tag::reference[]

== `b2::args::add_arg`

====
[horizontal]
Jam:: `rule add-arg ( name : opts + : help : flags * )`
{CPP}:: `void add_arg(const value_ref & name, list_cref opts, const value_ref &
help, list_cref flags);`
====

Does something.

end::reference[] */
void add_arg(const value_ref & name,
	list_cref opts,
	const value_ref & help,
	list_cref flags);

/* tag::reference[]

== `b2::args::get_arg`

====
[horizontal]
Jam:: `rule get-arg ( name )`
{CPP}:: `list_ref get_arg(value_ref name);`
====

Does something.

end::reference[] */
list_ref get_arg(const value_ref & name);

void add_args(int argc, char ** argv);

struct args_module : b2::bind::module_<args_module>
{
	const char * module_name = "arga";
	static const char * init_code;

	template <class Binder>
	void def(Binder & binder)
	{
		binder
			.def(&add_arg, "add-arg",
				"name" * _1 | "opts" * _1n | "help" * _1 | "flags" * _n)
			.def(&get_arg, "get-arg", "name" * _1);
		binder.eval(init_code);
		binder.loaded();
	}
};

}} // namespace b2::args

#endif
