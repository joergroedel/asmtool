#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "asmfile.h"
#include "helper.h"

using namespace std;

int parse(std::istream &is)
{
	string line;
	asmfile file;

	while (!is.eof()) {
		getline(is, line);

		line = trim(strip_comment(line));

		if (line == "")
			continue;

		file.add_statement(statement(line));
	}

	file.analyze();

	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		cerr << "File parameter required" << endl;
		return 1;
	}

	ifstream file(argv[1]);
	if (!file.is_open())
		cerr << "File not open: " << argv[1] << endl;

	parse(file);
	file.close();

	return 0;
}
