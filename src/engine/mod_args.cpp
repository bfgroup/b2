/*
Copyright 2024 René Ferdinand Rivera Morell
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or https://www.bfgroup.xyz/b2/LICENSE.txt)
*/

#include "mod_args.h"

#include "ext_bfgroup_lyra.h"
#include "jam.h"
#include "lists.h"
#include "output.h"
#include "startup.h"
#include "value.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace b2 { namespace args {

namespace {
struct args_reg
{
	static args_reg & ref()
	{
		static args_reg r;
		return r;
	}

	lyra::cli cli;
	std::unique_ptr<lyra::parse_result> result;
	std::unordered_map<std::string, std::shared_ptr<list_ref>> options;
	bool show_help = false;
	std::vector<std::string> args;
	bool need_reparse = true;

	args_reg() { cli.add_argument(lyra::help(show_help)); }

	void reparse()
	{
		need_reparse = false;
		// We can call this multiple times. Hence we go back to a clean state
		// on the collected values to get reproducible parsing.
		result.reset();
		for (auto & o : options)
		{
			o.second->reset();
		}
		show_help = false;

		result.reset(new lyra::parse_result(
			cli.parse(lyra::args(args.begin(), args.end()))));
		if (!(*result))
		{
			err_printf("[ERROR] %s\n", result->message().c_str());
		}
		if (show_help || !(*result))
		{
			std::ostringstream out;
			out << cli;
			err_puts(out.str().c_str());
			b2::clean_exit(EXITBAD);
		}
	}

	void add_opt(const value_ref & name,
		list_cref opts,
		const value_ref & help,
		list_cref flags)
	{
		if (options.count(name) > 0) return;
		need_reparse = true;
		std::shared_ptr<list_ref> values = std::make_shared<list_ref>();
		lyra::opt arg([values](const std::string & v) { values->push_back(v); },
			name->str());
		arg.help(help);
		for (auto opt : opts)
		{
			arg.name(opt->str());
		}
		cli.add_argument(arg);
	}

	list_ref get_opt(const value_ref & name)
	{
		if (need_reparse) reparse();
		if (options.count(name->str()) > 0) return *options[name->str()];
		return list_ref();
	}
};
} // namespace

void add_arg(const value_ref & name,
	list_cref opts,
	const value_ref & help,
	list_cref flags)
{
	args_reg::ref().add_opt(name, opts, help, flags);
}

list_ref get_arg(const value_ref & name)
{
	return args_reg::ref().get_opt(name);
}

void add_args(int argc, char ** argv) { args_reg::ref().parse(argc, argv); }

const char * args_module::init_code = R"jam(
rule __test__ ( )
{
    import assert ;
}
)jam";

}} // namespace b2::args
