/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>

#include <getopt.h>

#include "asmfile.h"
#include "helper.h"
#include "diff.h"

#include "newdata.h"

using namespace std;

asm_file *load_file(const char *name)
{
	ifstream in(name);
	asm_file *file;

#if 1
	{
		assembly::asm_file f(name);
		f.load();

		f.for_each_symbol([&f](string s, assembly::asm_symbol info) {
				string scope;
				string type;

				if (info.m_scope == assembly::symbol_scope::LOCAL)
					scope = "Local";
				else if (info.m_scope == assembly::symbol_scope::GLOBAL)
					scope = "Global";
				else
					scope = "Unknown";

				if (info.m_type == assembly::symbol_type::FUNCTION)
					type = "Function";
				else if (info.m_type == assembly::symbol_type::OBJECT)
					type = "Object";
				else
					type = "Unknown";

				cout << left;
				cout << "Symbol " << setw(40) << s;
				cout << "Scope " << setw(20) << scope;
				cout << "Type " << setw(10) << type;
				cout << "Idx" << info.m_idx << endl;

				if (info.m_type == assembly::symbol_type::FUNCTION) {
					std::unique_ptr<assembly::asm_function>
						fn(f.get_function(s, static_cast<assembly::func_flags>(assembly::func_flags::STRIP_DEBUG | assembly::func_flags::NORMALIZE)));

					if (fn == nullptr)
						return;

					fn->for_each_statement([](assembly::asm_statement& stmt) {
						cout << '\t' << stmt.serialize() << endl;
					});
				}
		});

	}
#endif

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
			if (it->size() > 0) {
				file->add_statement(asm_statement(*it));

#if 0
				std::unique_ptr<assembly::asm_statement> st = assembly::parse_statement(*it);
				cout << '[' << *it << "] [" << st->serialize() << ']' << endl;
#endif
			}
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

static int do_diff(const char *cmd, int argc, char **argv)
{
	struct diff_options diff_opts;
	asm_file *file1, *file2;
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

	file1 = load_file(argv[optind++]);
	file2 = load_file(argv[optind++]);

	diff(file1, file2, cout, diff_opts);

	delete file1;
	delete file2;

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
