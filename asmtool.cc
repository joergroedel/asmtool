/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
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

#include "callgraph.h"
#include "assembly.h"
#include "helper.h"
#include "diff.h"
#include "copy.h"
#include "info.h"
#include "show.h"

void usage(const char *cmd)
{
	std::cout << "Usage: " << cmd << " <subcommand> <options>" << std::endl;
	std::cout << "Available subcommands:" << std::endl;
	std::cout << "        diff          - Show changed functions between assembly files" << std::endl;
	std::cout << "        copy          - Copy specific symbols out of assembly files" << std::endl;
	std::cout << "        info          - Print info about symbols in an assembly file" << std::endl;
	std::cout << "        show          - Print assembly of a symbol as parsed by the tool" << std::endl;
	std::cout << "        cg, callgraph - Generate a call-graph from assembly" << std::endl;
	std::cout << "        help          - Print this message" << std::endl;
}

enum {
	OPTION_DIFF_HELP,
	OPTION_DIFF_SHOW,
	OPTION_DIFF_FULL,
	OPTION_DIFF_COLOR,
	OPTION_DIFF_NO_COLOR,
	OPTION_DIFF_PRETTY,
	OPTION_COPY_HELP,
	OPTION_COPY_OUTPUT,
	OPTION_INFO_HELP,
	OPTION_INFO_VERBOSE,
	OPTION_INFO_FUNCTIONS,
	OPTION_INFO_OBJECTS,
	OPTION_INFO_GLOBAL,
	OPTION_INFO_LOCAL,
	OPTION_INFO_ALL,
	OPTION_SHOW_HELP,
	OPTION_CG_HELP,
	OPTION_CG_OUTPUT,
	OPTION_CG_EXTERNAL,
};

static struct option diff_options[] = {
	{ "help",	no_argument,		0, OPTION_DIFF_HELP	},
	{ "show",	no_argument,		0, OPTION_DIFF_SHOW	},
	{ "full",	no_argument,		0, OPTION_DIFF_FULL	},
	{ "pretty",	no_argument,		0, OPTION_DIFF_PRETTY	},
	{ "color",	no_argument,		0, OPTION_DIFF_COLOR	},
	{ "no-color",	no_argument,		0, OPTION_DIFF_COLOR	},
	{ 0,		0,			0, 0			}
};

static void usage_diff(const char *cmd)
{
	std::cout << "Usage: " << cmd << " diff [options] old_file new_file" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "    --help, -h    - Print this help message" << std::endl;
	std::cout << "    --show, -s    - Show differences between functions" << std::endl;
	std::cout << "    --full, -f    - Print diff of full function" << std::endl;
	std::cout << "    --pretty, -p  - Print a side-by-side diff" << std::endl;
	std::cout << "    --color, -c   - Print diff in colors" << std::endl;
	std::cout << "    --no-color,   - Use no colors" << std::endl;
	std::cout << "    -U <num>      - Lines of context around changes" << std::endl;
}

static int do_diff(const char *cmd, int argc, char **argv)
{
	struct diff_options diff_opts;
	int c;

	diff_opts.color = isatty(fileno(stdout));

	while (true) {
		int opt_idx;

		c = getopt_long(argc, argv, "hsfU:pc", diff_options, &opt_idx);
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
			diff_opts.context = std::max(atoi(optarg), 0);
			break;
		case OPTION_DIFF_COLOR:
		case 'c':
			diff_opts.color = true;
			break;
		case OPTION_DIFF_NO_COLOR:
			diff_opts.color = false;
			break;
		default:
			usage_diff(cmd);
			return 1;
		}
	}

	if (optind + 2 > argc) {
		std::cerr << "Two file parameters required" << std::endl;
		usage_diff(cmd);
		return 1;
	}

	std::string filename1 = argv[optind++];
	std::string filename2 = argv[optind++];

	auto pos1 = filename1.find_first_of(":");
	auto pos2 = filename2.find_first_of(":");

	if (pos1 != std::string::npos && pos2 != std::string::npos) {
		std::string objname1, objname2;

		objname1  = filename1.substr(pos1 + 1);
		objname2  = filename2.substr(pos2 + 1);
		filename1 = filename1.substr(0, pos1);
		filename2 = filename2.substr(0, pos2);

		diff_functions(filename1, filename2, objname1, objname2, diff_opts);
	} else {
		diff_files(filename1.c_str(), filename2.c_str(), diff_opts);
	}


	return 0;
}

static struct option copy_options[] = {
	{ "help",	no_argument,		0, OPTION_COPY_HELP	},
	{ "output",	required_argument,	0, OPTION_COPY_OUTPUT	},
	{ 0,		0,			0, 0			}
};

static void usage_copy(const char *cmd)
{
	std::cout << "Usage: " << cmd << " copy [options] filename [functions...]" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "    --help, -h              - Print this help message" << std::endl;
	std::cout << "    --output, -o <filename> - Destination file, default is stdout" << std::endl;
}

static int do_copy(const char *cmd, int argc, char **argv)
{
	std::string output_file, input_file;
	std::vector<std::string> symbols;

	while (true) {
		int opt_idx, c;

		c = getopt_long(argc, argv, "ho:", copy_options, &opt_idx);
		if (c == -1)
			break;

		switch (c) {
		case OPTION_COPY_HELP:
		case 'h':
			usage_copy(cmd);
			return 0;
		case OPTION_COPY_OUTPUT:
		case 'o':
			output_file = optarg;
			break;
		}
	}

	if (optind + 2 > argc) {
		std::cerr << "Error: Filename and at least one symbol required" << std::endl;
		usage_copy(cmd);
		return 1;
	}

	input_file = argv[optind++];

	while (optind < argc)
		symbols.push_back(argv[optind++]);

	if (output_file.length() > 0) {
		std::ofstream of(output_file.c_str());

		copy_functions(input_file, symbols, of);
	} else {
		copy_functions(input_file, symbols, std::cout);
	}

	return 0;
}

static struct option info_options[] = {
	{ "help",	no_argument,		0, OPTION_INFO_HELP		},
	{ "verbose",	no_argument,		0, OPTION_INFO_VERBOSE		},
	{ "functions",	no_argument,		0, OPTION_INFO_FUNCTIONS	},
	{ "objects",	no_argument,		0, OPTION_INFO_OBJECTS		},
	{ "global",	no_argument,		0, OPTION_INFO_GLOBAL		},
	{ "local",	no_argument,		0, OPTION_INFO_LOCAL		},
	{ "all",	no_argument,		0, OPTION_INFO_ALL		},
	{ 0,		0,			0, 0				}
};

static void usage_info(const char *cmd)
{
	std::cout << "Usage: " << cmd << " info [options] filename[:function]" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "    --help, -h         - Print this help message" << std::endl;
	std::cout << "    --verbose          - Print symbols referenced by each function" << std::endl;
	std::cout << "    --functions, -f    - Print function-type symbols (default)" << std::endl;
	std::cout << "    --objects, -o      - Print object-type symbols" << std::endl;
	std::cout << "    --global, -g       - Print global symbols (default)" << std::endl;
	std::cout << "    --local, -l        - Print local symbols" << std::endl;
	std::cout << "    --all, -a          - Print all symbols" << std::endl;
}

static int do_info(const char *cmd, int argc, char **argv)
{
	bool f_cmd = false, g_cmd = false;
	std::string filename, fn_name;
	struct info_options opts;

	while (true) {
		int opt_idx, c;

		c = getopt_long(argc, argv, "hvfogla", info_options, &opt_idx);
		if (c == -1)
			break;

		switch (c) {
		case OPTION_INFO_HELP:
		case 'h':
			usage_info(cmd);
			return 0;
		case OPTION_INFO_VERBOSE:
		case 'v':
			opts.verbose = true;
			break;
		case OPTION_INFO_FUNCTIONS:
		case 'f':
			f_cmd = true;
			break;
		case OPTION_INFO_OBJECTS:
		case 'o':
			opts.functions = false;
			opts.objects   = true;
			break;
		case OPTION_INFO_GLOBAL:
		case 'g':
			g_cmd = true;
			break;
		case OPTION_INFO_LOCAL:
		case 'l':
			opts.global = false;
			opts.local  = true;
			break;
		case OPTION_INFO_ALL:
		case 'a':
			opts.global = opts.local = true;
			opts.functions = opts.objects = true;
			break;
		}
	}

	if (!opts.functions)
		opts.functions = f_cmd;

	if (!opts.global)
		opts.global = g_cmd;

	if (optind + 1 > argc) {
		std::cerr << "Error: Filename required" << std::endl;
		usage_copy(cmd);
		return 1;
	}

	filename = argv[optind++];

	auto pos = filename.find_first_of(":");

	if (pos != std::string::npos) {
		opts.fn_name = filename.substr(pos + 1);
		filename     = filename.substr(0, pos);
	}

	if (opts.fn_name == "")
		print_symbol_info(filename.c_str(), opts);
	else
		print_one_symbol_info(filename.c_str(), opts.fn_name);

	return 0;
}

static struct option show_options[] = {
	{ "help",	no_argument,		0, OPTION_INFO_HELP		},
	{ 0,		0,			0, 0				}
};

static void usage_show(const char *cmd)
{
	std::cout << "Usage: " << cmd << " show [options] filename symbol" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "    --help, -h         - Print this help message" << std::endl;
}

static int do_show(const char *cmd, int argc, char **argv)
{
	std::string filename, sym;

	while (true) {
		int opt_idx, c;

		c = getopt_long(argc, argv, "h", show_options, &opt_idx);
		if (c == -1)
			break;

		switch (c) {
		case OPTION_INFO_HELP:
		case 'h':
			usage_show(cmd);
			return 0;
		default:
			usage_show(cmd);
			return 1;
		}
	}

	if (optind + 1 > argc) {
		std::cerr << "Error: Filename and symbol required" << std::endl;
		usage_show(cmd);
		return 1;
	}

	filename = argv[optind++];

	if (optind < argc)
		sym = argv[optind++];

	auto pos = filename.find_first_of(":");

	if (sym == "" && pos != std::string::npos) {
		sym = filename.substr(pos + 1);
		filename = filename.substr(0, pos);
	}

	if (sym == "") {
		std::cerr << "Error: Symbol name required" << std::endl;
		usage_show(cmd);
		return 1;
	}

	show_symbol(filename.c_str(), sym);

	return 0;
}

static struct option cg_options[] = {
	{ "help",	no_argument,		0, OPTION_CG_HELP		},
	{ "output",	required_argument,	0, OPTION_CG_OUTPUT		},
	{ "external",	no_argument,		0, OPTION_CG_EXTERNAL		},
	{ 0,		0,			0, 0				}
};

static void usage_cg(const char *cmd)
{
	std::cout << "Usage: " << cmd << " callgraph [options] file(s)" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "    --help, -h          - Print this help message" << std::endl;
	std::cout << "    --output, -o <file> - Output filename (default: callgraph.dot)" << std::endl;
	std::cout << "    --external, -e      - Include external symbols in call-graph" << std::endl;
}

static int do_callgraph(const char *cmd, int argc, char **argv)
{
	struct cg_options opts;
	std::string filename;

	while (true) {
		int opt_idx, c;

		c = getopt_long(argc, argv, "ho:e", cg_options, &opt_idx);
		if (c == -1)
			break;

		switch (c) {
		case OPTION_CG_HELP:
		case 'h':
			usage_cg(cmd);
			return 0;
		case OPTION_CG_OUTPUT:
		case 'o':
			opts.output_file = optarg;
			break;
		case OPTION_CG_EXTERNAL:
		case 'e':
			opts.include_external = true;
			break;
		default:
			usage_cg(cmd);
			return 1;
		}
	}

	if (optind + 1 > argc) {
		std::cerr << "Error: Filename required" << std::endl;
		usage_show(cmd);
		return 1;
	}

	while (optind < argc)
		opts.input_files.emplace_back(argv[optind++]);

	generate_callgraph(opts);

	return 0;
}

int main(int argc, char **argv)
{
	std::string command;
	int ret = 0;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	command = argv[1];

	if (command == "diff")
		ret = do_diff(argv[0], argc - 1, argv + 1);
	else if (command == "copy")
		ret = do_copy(argv[0], argc - 1, argv + 1);
	else if (command == "info")
		ret = do_info(argv[0], argc - 1, argv + 1);
	else if (command == "show")
		ret = do_show(argv[0], argc - 1, argv + 1);
	else if (command == "cg" || command == "callgraph")
		ret = do_callgraph(argv[0], argc - 1, argv + 1);
	else if (command == "help")
		usage(argv[0]);
	else {
		std::cerr << "Unknown sub-command: " << command << std::endl;
		usage(argv[0]);
		ret = 1;
	}

	return ret;
}
