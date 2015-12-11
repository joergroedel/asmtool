#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "asmfile.h"
#include "helper.h"

using namespace std;

asmfile *load_file(const char *name)
{
	ifstream in(name);
	asmfile *asm_file;

	if (!in.is_open()) {
		cerr << "File not open: " << name << endl;
		return 0;
	}

	asm_file = new asmfile();

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
			asm_file->add_statement(asm_statement(*it));
	}

	asm_file->analyze();

	in.close();

	return asm_file;
}

void changes(asmfile *file1, asmfile *file2)
{
	for (vector<string>::const_iterator it = file2->functions_begin();
	     it != file2->functions_end();
	     it++) {
		if (!file1->has_function(*it)) {
			cout << "New function: " << *it << endl;
			continue;
		}

		asm_function *func1 = file1->get_function(*it);
		asm_function *func2 = file2->get_function(*it);

		if (!(*func1 == *func2))
			cout << "Changed function: " << *it << endl;

		delete func1;
		delete func2;
		func1 = func2 = 0;
	}
}

int main(int argc, char **argv)
{
	asmfile *file1, *file2;
	if (argc != 3) {
		cerr << "2 File parameters required" << endl;
		return 1;
	}

	file1 = load_file(argv[1]);
	file2 = load_file(argv[2]);

	changes(file1, file2);

	return 0;
}
