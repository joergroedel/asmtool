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

int main(int argc, char **argv)
{
	asm_file *file1, *file2;
	if (argc != 3) {
		cerr << "2 File parameters required" << endl;
		return 1;
	}

	file1 = load_file(argv[1]);
	file2 = load_file(argv[2]);

	diff(file1, file2, cout);

	return 0;
}
