/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __DIFF_H
#define __DIFF_H

#include <string>
#include <map>

#include "asmfile.h"

struct diff_options {
	bool show;
	bool full;
	int context;

	diff_options();
};

void diff(asm_file *file1, asm_file *file2, std::ostream &os,
	  struct diff_options &opts);
bool compare_statements(asm_file *file1, asm_statement &s1,
			asm_file *file2, asm_statement &s2,
			std::map<std::string, std::string> &symbol_map);
bool compare_functions(asm_file *file1, asm_function *f1,
		       asm_file *file2, asm_function *f2,
		       std::map<std::string, std::string> &symbol_map);

#endif
