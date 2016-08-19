/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <list>

#include <unistd.h>

#include "assembly.h"
#include "generic-diff.h"
#include "helper.h"
#include "diff.h"

// For caching diff results of individual symbols
struct diff_result {
	std::string symbol1;
	std::string symbol2;
	bool flat_diff;

	diff_result()
		: flat_diff(false)
	{
	}
};

// Diff dependency chain
struct diff_chain;

using sym_list = std::list<struct diff_chain>;

struct diff_chain {
	std::string symbol1;
	std::string symbol2;
	bool flat_diff;
	bool deep_diff;
	sym_list list;

	diff_chain(std::string s1, std::string s2)
		: symbol1(s1), symbol2(s2), flat_diff(true), deep_diff(true)
	{
	}
};

using result_map = std::map<std::string, struct diff_result>;

diff_options::diff_options()
	: show(false), pretty(false), color(true), context(3)
{ }

static void print_diff_line(assembly::asm_function &fn1,
			    assembly::asm_function &fn2,
			    diff::diff_element &item,
			    struct diff_options &opts)
{
	static const char *reset = "\033[0m";
	static const char *black = "\033[30m";
	static const char *red   = "\033[31m";
	static const char *green = "\033[32m";

	const char *color = "", *no_color = "";
	std::string str0, str1, str2;
	bool colors = opts.color;
	char c = ' ';

	no_color = colors ? reset : "";

	switch (item.type) {
	case diff::diff_type::EQUAL:
		c     = ' ';
		color = colors ? black : "";
		str0  = fn1.element(item.idx_a).statement();
		str1  = fn1.element(item.idx_a).statement();
		str2  = fn2.element(item.idx_b).statement();
		break;
	case diff::diff_type::ADDED:
		c     = '+';
		color = colors ? green : "";
		str0  = fn2.element(item.idx_b).statement();
		str1  = "";
		str2  = fn2.element(item.idx_b).statement();
		break;
	case diff::diff_type::REMOVED:
		c     = '-';
		color = colors ? red : "";
		str0  = fn1.element(item.idx_a).statement();
		str1  = fn1.element(item.idx_a).statement();
		str2  = "";
		break;
	}

	str0 = expand_tab(trim(str0));
	str1 = expand_tab(trim(str1));
	str2 = expand_tab(trim(str2));

	if (str1.size() >= 40)
		str1 = str1.substr(0, 34) + "[...]";

	if (str2.size() >= 40)
		str2 = str2.substr(0, 34) + "[...]";

	std::cout << color;

	if (opts.pretty)
		std::cout << "         " << std::setw(40) << str1 << "| " << str2;
	else
		std::cout << "        " << c << str0;

	std::cout << no_color << std::endl;
}

static void print_diff(assembly::asm_function &fn1, assembly::asm_function &fn2,
		       assembly::asm_diff &diff, struct diff_options &opts)
{
	auto diff_info = diff.get_diff();
	auto size = diff_info.size();
	decltype(size) context = opts.context;
	decltype(size) i, to_print = 0;

	// Initialize to_print
	for (i = 0; i < context + 1; ++i) {
		if (i == size)
			break;

		if (diff_info[i].type != diff::diff_type::EQUAL)
			to_print = i + context + 1;
	}

	for (i = 0; i < size; ++i) {
		if (to_print) {
			print_diff_line(fn1, fn2, diff_info[i], opts);
			to_print -= 1;
		}

		auto next = std::min(i + context + 1, size - 1);

		if (diff_info[next].type != diff::diff_type::EQUAL) {
			if (!to_print)
				std::cout << "         [...]" << std::endl;

			to_print = (2 * context) + 1;
		}
	}
}

static void compare(const assembly::asm_file &file1,
		    const assembly::asm_file &file2,
		    std::string fname1,
		    std::string fname2,
		    result_map &results,
		    struct diff_chain &chain);

static bool compare_symbol_map(const assembly::asm_file &file1,
			       const assembly::asm_file &file2,
			       assembly::symbol_map &map,
			       result_map &results,
			       struct diff_chain &chain)
{
	bool ret = true;

	for (auto it = map.begin(), end = map.end(); it != end; ++it) {
		// Limit comparison to functions for now
		if (!file1.has_function(it->second) || !file2.has_function(it->first))
			continue;

		struct diff_chain nested(it->second, it->first);

		compare(file1, file2, it->second, it->first, results, nested);

		chain.list.push_back(nested);

		ret = ret && nested.deep_diff;
	}

	return ret;
}

static void compare(const assembly::asm_file &file1,
		    const assembly::asm_file &file2,
		    std::string fname1,
		    std::string fname2,
		    result_map &results,
		    struct diff_chain &chain)
{
	// First check if we already compared these functions
	if (results.find(fname2) != results.end()) {
		chain.deep_diff = chain.flat_diff = results[fname2].flat_diff;
		return;
	}

	results[fname2].symbol1 = fname1;
	results[fname2].symbol2 = fname2;

	constexpr auto flags = assembly::func_flags::STRIP_DEBUG | assembly::func_flags::NORMALIZE;

	// We didn't, run compare
	std::unique_ptr<assembly::asm_function> fn1(file1.get_function(fname1, flags));
	std::unique_ptr<assembly::asm_function> fn2(file2.get_function(fname2, flags));

	if (fn1 == nullptr || fn2 == nullptr) {
		results[fname2].flat_diff = false;
		return;
	}

	assembly::asm_diff compare(*fn1, *fn2);

	if (compare.is_different()) {
		results[fname2].flat_diff = false;
		chain.flat_diff = false;
		chain.deep_diff = false;
	} else {
		assembly::symbol_map map;

		// Flat diff didn't show any differences
		results[fname2].flat_diff = true;
		chain.flat_diff = true;

		// We need to set deep_diff to true here because it might be
		// used in compare_symbol_map when we have functions that call
		// each other recursivly, so make sure it has a defined value
		// there. We update it to the real value when compare_symbol_map
		// returns.
		chain.deep_diff = true;

		fn2->get_symbol_map(map, *fn2);

		chain.deep_diff = compare_symbol_map(file1, file2, map, results, chain);
	}
}

void print_diff_chain(struct diff_chain &chain, std::string indent = "")
{
	std::cout << indent << "-> " << chain.symbol2;
	if (chain.symbol1 != chain.symbol2)
		std::cout << "(was " << chain.symbol1 << ")";
	std::cout << "[" << (chain.flat_diff ? "=" : "!") << "]" << std::endl;

	for (auto l = chain.list.begin(), end = chain.list.end(); l != end; ++l) {
		print_diff_chain(*l, indent + "    ");
	}
}

void diff_files(const char *fname1, const char *fname2, struct diff_options &opts)
{
	assembly::asm_file file1(fname1);
	assembly::asm_file file2(fname2);

	try {
		std::vector<std::string> f1_functions, f2_functions;
		std::map<std::string, struct diff_result> results;

		file1.load();
		file2.load();

#if 0
		file2.for_each_symbol([](std::string sym, assembly::asm_symbol info) {
			std::string scope;
			std::string type;

			switch (info.m_scope) {
			case assembly::symbol_scope::LOCAL:
				scope = "Local";
				break;
			case assembly::symbol_scope::GLOBAL:
				scope = "Global";
				break;
			default:
				scope = "Unknown";
				break;
			}

			switch (info.m_type) {
			case assembly::symbol_type::FUNCTION:
				type = "Function";
				break;
			case assembly::symbol_type::OBJECT:
				type = "Object";
				break;
			default:
				type = "Unknown";
				break;
			}

			std::cout << std::left;
			std::cout << "Symbol: " << std::setw(48) << sym;
			std::cout << " Scope: " << std::setw(10) << scope;
			std::cout << " Type: "  << std::setw(10) << type << std::endl;
		});
#endif

		// Get function lists from input files
		file1.for_each_symbol([&f1_functions](std::string symbol, assembly::asm_symbol info) {
			if (!generated_symbol(symbol) &&
			    info.m_type == assembly::symbol_type::FUNCTION)
				f1_functions.push_back(symbol);
		});

		std::sort(f1_functions.begin(), f1_functions.end());

		file2.for_each_symbol([&f2_functions](std::string symbol, assembly::asm_symbol info) {
			if (!generated_symbol(symbol) &&
			    info.m_type == assembly::symbol_type::FUNCTION)
				f2_functions.push_back(symbol);
		});

		std::sort(f2_functions.begin(), f2_functions.end());

		// Now check the functions and diff them
		constexpr auto flags = assembly::func_flags::STRIP_DEBUG | assembly::func_flags::NORMALIZE;

		for (auto it = f2_functions.begin(), end = f2_functions.end(); it != end; ++it) {

			if (!binary_search(f1_functions.begin(), f1_functions.end(), *it)) {
				std::cout << std::setw(20) << "New function: " << *it << std::endl;
				continue;
			}

			std::unique_ptr<assembly::asm_function> fn1(file1.get_function(*it, flags));
			std::unique_ptr<assembly::asm_function> fn2(file2.get_function(*it, flags));

			if (fn1 == nullptr || fn2 == nullptr)
				continue;

			assembly::asm_diff compare(*fn1, *fn2);

			if (compare.is_different()) {
				results[*it].symbol1 = *it;
				results[*it].symbol2 = *it;
				results[*it].flat_diff  = false;

				std::cout << std::left;
				std::cout << std::setw(20) << "Changed function: " << *it << std::endl;

				if (opts.show)
					print_diff(*fn1, *fn2, compare, opts);
			} else {
				// Functions are apparently identical - but the
				// difference might be in the compiler-generated
				// symbols they reference.  Check for that.
				struct diff_chain chain(*it, *it);
				assembly::symbol_map map;

				results[*it].symbol1 = *it;
				results[*it].symbol2 = *it;
				results[*it].flat_diff = true;

				fn2->get_symbol_map(map, *fn2);

				if (!compare_symbol_map(file1, file2, map, results, chain)) {
					std::ostringstream indent;
					indent << std::left << std::setw(20) << "";

					std::cout << std::left;
					std::cout << std::setw(20) << "Changed function: " << *it << std::endl;
					std::cout << indent.str() << "(Only referenced compiler-generated symbols changed)";
					std::cout << std::endl;
					std::cout << indent.str() << "Dependency chain:" << std::endl;
					print_diff_chain(chain, indent.str());
				}
			}
		}

		// Done with the diffs - now search for removed functions
		for (auto it = f1_functions.begin(), end = f1_functions.end(); it != end; ++it) {

			if (!binary_search(f2_functions.begin(), f2_functions.end(), *it)) {
				std::cout << std::setw(20) << "Removed function: " << *it << std::endl;
				continue;
			}
		}

	} catch (std::runtime_error &e) {
		std::cerr << "Failed to diff files: " << e.what() << std::endl;
	}
}

