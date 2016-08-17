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
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>

#include <getopt.h>
#include <unistd.h>

#include "asmfile.h"
#include "helper.h"
#include "diff.h"

#include "assembly.h"

using namespace std;

asm_file *load_file(const char *name)
{
	ifstream in(name);
	asm_file *file;

	if (!in.is_open()) {
		cerr << "File not open: " << name << endl;
		return 0;
	}

	file = new asm_file();

	while (!in.eof()) {
		string line;

		getline(in, line);

		line = trim(strip_comment(line));

		if (line == "")
			continue;

		vector<string> lines = split_trim(";", line);

		for (auto it = lines.begin(); it != lines.end(); it++) {
			if (it->size() > 0)
				file->add_statement(asm_statement(*it));
		}
	}

	file->analyze();

	in.close();

	return file;
}

void usage(const char *cmd)
{
	cout << "Usage: " << cmd << " <subcommand> <options>" << endl;
	cout << "Available subcommands:" << endl;
	cout << "        diff - Show changed functions between assembly files" << endl;
	cout << "        help - Print this message" << endl;
}

enum {
	OPTION_DIFF_HELP,
	OPTION_DIFF_SHOW,
	OPTION_DIFF_FULL,
	OPTION_DIFF_PRETTY,
};

static struct option diff_options[] = {
	{ "help",	no_argument,		0, OPTION_DIFF_HELP	},
	{ "show",	no_argument,		0, OPTION_DIFF_SHOW	},
	{ "full",	no_argument,		0, OPTION_DIFF_FULL	},
	{ "pretty",	no_argument,		0, OPTION_DIFF_PRETTY	},
	{ 0,		0,			0, 0			}
};

static void usage_diff(const char *cmd)
{
	cout << "Usage: " << cmd << " diff [options] old_file new_file" << endl;
	cout << "Options:" << endl;
	cout << "    --help, -h    - Print this help message" << endl;
	cout << "    --show, -s    - Show differences between functions" << endl;
	cout << "    --full, -f    - Print diff of full function" << endl;
	cout << "    --pretty, -p  - Print a side-by-side diff" << endl;
	cout << "    -U <num>      - Lines of context around changes" << endl;
}

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
		std::cout << "         " << setw(40) << str1 << "| " << str2;
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

static void diff_files(const char *fname1, const char *fname2, struct diff_options &opts)
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
				std::cout << setw(20) << "New function: " << *it << std::endl;
				continue;
			}

			std::unique_ptr<assembly::asm_function> fn1(file1.get_function(*it, flags));
			std::unique_ptr<assembly::asm_function> fn2(file2.get_function(*it, flags));

			if (fn1 == nullptr || fn2 == nullptr)
				continue;

			assembly::asm_diff compare(*fn1, *fn2);

			if (compare.is_different()) {
				std::cout << left;
				std::cout << setw(20) << "Changed function: " << *it << std::endl;

				if (opts.show)
					print_diff(*fn1, *fn2, compare, opts);
			}
		}

		// Done with the diffs - now search for removed functions
		for (auto it = f1_functions.begin(), end = f1_functions.end(); it != end; ++it) {

			if (!binary_search(f2_functions.begin(), f2_functions.end(), *it)) {
				std::cout << setw(20) << "Removed function: " << *it << std::endl;
				continue;
			}
		}

	} catch (std::runtime_error &e) {
		std::cerr << "Failed to diff files: " << e.what() << std::endl;
	}
}

static int do_diff(const char *cmd, int argc, char **argv)
{
	struct diff_options diff_opts;
	int c;

	while (true) {
		int opt_idx;

		c = getopt_long(argc, argv, "hsfU:p", diff_options, &opt_idx);
		if (c == -1)
			break;

		switch (c) {
		case OPTION_DIFF_HELP:
		case 'h':
			usage_diff(cmd);
			return 0;
		case OPTION_DIFF_SHOW:
		case 's':
			diff_opts.show = true;
			break;
		case OPTION_DIFF_FULL:
		case 'f':
			diff_opts.context = 1 << 16;
			break;
		case OPTION_DIFF_PRETTY:
		case 'p':
			diff_opts.pretty = true;
			break;
		case 'U':
			diff_opts.context = max(atoi(optarg), 0);
			break;
		default:
			usage_diff(cmd);
			return 1;
		}
	}

	if (optind + 2 > argc) {
		cerr << "Two file parameters required" << endl;
		usage_diff(cmd);
		return 1;
	}

	std::string fname1 = argv[optind++];
	std::string fname2 = argv[optind++];

#if 0
	{
		asm_file *file1, *file2;

		file1 = load_file(fname1.c_str());
		file2 = load_file(fname2.c_str());

		diff(file1, file2, cout, diff_opts);

		delete file1;
		delete file2;
	}
#endif

	diff_files(fname1.c_str(), fname2.c_str(), diff_opts);

	return 0;
}

int main(int argc, char **argv)
{
	string command;
	int ret = 0;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	command = argv[1];

	if (command == "diff")
		ret = do_diff(argv[0], argc - 1, argv + 1);
	else if (command == "help")
		usage(argv[0]);
	else {
		cerr << "Unknown sub-command: " << command << endl;
		usage(argv[0]);
		ret = 1;
	}

	return ret;
}
