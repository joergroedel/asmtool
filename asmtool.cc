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

#include "assembly.h"
#include "helper.h"
#include "diff.h"


using namespace std;

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
