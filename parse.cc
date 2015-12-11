#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "asmfile.h"
#include "helper.h"

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

static bool compare_param(asm_file *file1, asm_param& param1,
			  asm_file *file2, asm_param &param2)
{
	size_t size1, size2;
	bool ret = true;

	size1 = param1.tokens.size();
	size2 = param2.tokens.size();

	if (size1 != size2)
		return false;

	for (size_t i = 0; i < size1; i++) {
		asm_token &t1 = param1.tokens[i];
		asm_token &t2 = param2.tokens[i];

		if (t1.type != t2.type) {
			ret = false;
			continue;
		}

		if (t1.token == t2.token)
			continue;

		ret = false;
	}

	return ret;
}

static bool compare_instructions(asm_file *file1, asm_instruction *ins1,
				 asm_file *file2, asm_instruction *ins2)
{
	size_t size1, size2;
	int ret = true;

	if (ins1->instruction != ins2->instruction)
		return false;

	size1 = ins1->params.size();
	size2 = ins2->params.size();

	if (size1 != size2)
		return false;

	/* Compare parameters */
	for (size_t i = 0; i < size1; ++i) {
		if (!compare_param(file1, ins1->params[i],
				   file2, ins2->params[i]))
			ret = false;
	}

	return ret;
}

bool compare_functions(asm_file *file1, asm_function *f1,
		       asm_file *file2, asm_function *f2)
{
	size_t size1 = f1->statements.size();
	size_t size2 = f2->statements.size();
	bool ret = true;

	/* First check if the functions have the same size */
	if (size1 != size2)
		return false;

	/* Now iterate over the function statements */
	for (size_t i = 0; i < size1; ++i) {
		asm_statement &s1 = f1->statements[i];
		asm_statement &s2 = f2->statements[i];

		if (s1.type != s2.type) {
			ret = false;
			continue;
		}

		/*
		 * First check for labels. They need to be identical because
		 * they are normalized between the two versions of the function
		 */
		if ((s1.type == LABEL) &&
		    (s1.obj_label->label != s2.obj_label->label)) {
			ret = false;
			continue;
		}

		if (s1.type == INSTRUCTION) {
			if (!compare_instructions(file1, s1.obj_instruction,
						  file2, s2.obj_instruction)) {
				ret = false;
				continue;
			}
		}
	}

	return ret;
}

void changes(asm_file *file1, asm_file *file2)
{
	for (vector<string>::const_iterator it = file2->functions.begin();
	     it != file2->functions.end();
	     it++) {
		if (!file1->has_function(*it)) {
			cout << "New function: " << *it << endl;
			continue;
		}

		asm_function *func1 = file1->get_function(*it);
		asm_function *func2 = file2->get_function(*it);

		if (!compare_functions(file1, func1, file2, func2))
			cout << "Changed function: " << *it << endl;

		delete func1;
		delete func2;
		func1 = func2 = 0;
	}
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

	changes(file1, file2);

	return 0;
}
