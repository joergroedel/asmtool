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
#include <string>
#include <vector>

#include <unistd.h>

#include "assembly.h"
#include "generic-diff.h"
#include "helper.h"
#include "diff.h"

diff_options::diff_options()
	: show(false), pretty(false), context(3)
{ }

static void print_diff_line(assembly::asm_function &fn1,
			    assembly::asm_function &fn2,
			    differences::diff_element &item,
			    struct diff_options &opts)
{
	static const char *reset = "\033[0m";
	static const char *black = "\033[30m";
	static const char *red   = "\033[31m";
	static const char *green = "\033[32m";

	bool colors = isatty(fileno(stdout));
	std::string str0, str1, str2;
	const char *color = "", *no_color = "";
	char c = ' ';

	no_color = colors ? reset : "";

	switch (item.type) {
	case differences::diff_type::EQUAL:
		c     = ' ';
		color = colors ? black : "";
		str0  = fn1.element(item.idx_a).statement();
		str1  = fn1.element(item.idx_a).statement();
		str2  = fn2.element(item.idx_b).statement();
		break;
	case differences::diff_type::ADDED:
		c     = '+';
		color = colors ? green : "";
		str0  = fn2.element(item.idx_b).statement();
		str1  = "";
		str2  = fn2.element(item.idx_b).statement();
		break;
	case differences::diff_type::REMOVED:
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

		if (diff_info[i].type != differences::diff_type::EQUAL)
			to_print = i + context + 1;
	}

	for (i = 0; i < size; ++i) {
		if (to_print) {
			print_diff_line(fn1, fn2, diff_info[i], opts);
			to_print -= 1;
		}

		auto next = std::min(i + context + 1, size - 1);

		if (diff_info[next].type != differences::diff_type::EQUAL) {
			if (!to_print)
				std::cout << "         [...]" << std::endl;

			to_print = (2 * context) + 1;
		}
	}
}

void diff_files(const char *fname1, const char *fname2, struct diff_options &opts)
{
	assembly::asm_file file1(fname1);
	assembly::asm_file file2(fname2);

	try {
		std::vector<std::string> f1_functions, f2_functions;

		file1.load();
		file2.load();

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
				std::cout << std::left;
				std::cout << std::setw(20) << "Changed function: " << *it << std::endl;

				if (opts.show)
					print_diff(*fn1, *fn2, compare, opts);
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

