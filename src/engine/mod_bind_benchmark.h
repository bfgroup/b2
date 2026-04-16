/*
Copyright 2026 Paolo Pastori
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

namespace b2 {

//void timed_no_args();
value_ref timed_no_args();
value_ref timed_any_args(list_cref args);

/*
 * New binding style.
 */
struct benchmark_module : b2::bind::module_<benchmark_module>
{
	const char * module_name = "benchmark";

	template <class Binder>
	void def(Binder & binder)
	{
		//binder.def(&b2::timed_no_args, "timed");
		binder.def(&b2::timed_any_args, "timed");
		binder.loaded();
	}
};
} // namespace b2
