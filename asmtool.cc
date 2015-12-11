/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "asmfile.h"
#include "helper.h"
#include "diff.h"

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

		for (vector<string>::iterator it = lines.begin();
		     it != lines.end();
		     it++)
			file->add_statement(asm_statement(*it));
	}

	file->analyze();

	in.close();

	return file;
}

static int command_diff(int argc, char **argv)
{
	asm_file *file1, *file2;
	bool verbose = false;

	while (argc) {
		string option;

		argv += 1;
		argc -= 1;

		option = argv[0];
		if (!option.size())
			continue;

		if (option[0] != '-')
			break;

		if (option == "-v") {
			verbose = true;
		} else {
			cerr << "Unknown option: " << option << endl;
			return 1;
		}
	}

	if (argc != 2) {
		cerr << "2 File parameters required" << endl;
		return 1;
	}

	file1 = load_file(argv[0]);
	file2 = load_file(argv[1]);

	if (!file1 || !file2)
		return 1;

	diff(file1, file2, cout);

	return 0;
}

void usage(const char *cmd)
{
	cout << "Usage: " << cmd << " <subcommand> <options>" << endl;
	cout << "Available subcommands:" << endl;
	cout << "        diff - Show changed functions between assembly files" << endl;
	cout << "        help - Print this message" << endl;
}

int main(int argc, char **argv)
{
	string command;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	command = argv[1];

	if (command == "diff")
		return command_diff(argc - 1, argv + 1);
	else if (command == "help")
		usage(argv[0]);

	return 0;
}
