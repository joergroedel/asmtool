/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "asmfile.h"
#include "helper.h"
#include "diff.h"

using namespace std;

static bool compare_param(asm_file *file1, asm_param& param1,
			  asm_file *file2, asm_param &param2,
			  map<string, string> &symbol_map)
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

		if (t1.type != t2.type)
			goto next_false;

		if (t1.type == asm_token::IDENTIFIER) {
			if (symbol_map.find(t2.token) != symbol_map.end()) {
				if (symbol_map[t2.token] != t1.token) {
					cout << "WARNING: " << t2.token;
					cout << " maps to " << symbol_map[t2.token];
					cout << " and " << t1.token << endl;
				}
			} else {
				symbol_map[t2.token] = t1.token;
			}
		}

		if (t1.token == t2.token)
			continue;

		if (t1.type != asm_token::IDENTIFIER)
			goto next_false;

		if (!generated_symbol(t1.token) ||
		    !generated_symbol(t2.token))
			goto next_false;

		continue;

	next_false:
		ret = false;
	}

	return ret;
}

static bool compare_instructions(asm_file *file1, asm_instruction *ins1,
				 asm_file *file2, asm_instruction *ins2,
				 map<string, string> &symbol_map)
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
				   file2, ins2->params[i],
				   symbol_map))
			ret = false;
	}

	return ret;
}

bool compare_functions(asm_file *file1, asm_function *f1,
		       asm_file *file2, asm_function *f2,
		       map<string, string> &symbol_map)
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
						  file2, s2.obj_instruction,
						  symbol_map)) {
				ret = false;
				continue;
			}
		}

		if (s1.type != LABEL && s1.type != INSTRUCTION) {
			unsigned int i;

			if (s1.params.size() != s2.params.size()) {
				ret = false;
				continue;
			}

			for (i = 0; i < s1.params.size(); i++) {
				if (s1.params[i] != s2.params[i]) {
					ret = false;
					break;
				}
			}
		}
	}


	return ret;
}

static bool function_list(string type, const vector<string> &list, ostream &os)
{
	if (!list.size())
		return false;

	os << type << " functions:" << endl;

	for (vector<string>::const_iterator it = list.begin();
	     it != list.end();
	     it++) {
		os << "        " << *it << endl;
	}

	return true;
}

static bool compare_symbol_map(asm_file *file1, asm_file *file2,
			       map<string, string> &symbol_map)
{
	bool changed = false;

	for (map<string, string>::iterator symbol = symbol_map.begin();
			symbol != symbol_map.end();
			++symbol) {
		string sym_old(symbol->second), sym_new(symbol->first);

		if (!generated_symbol(sym_old) &&
		    !generated_symbol(sym_new))
			continue;

		if (!file1->has_function(sym_old) ||
		    !file2->has_function(sym_new))
			continue;

		asm_function *func_old = file1->get_function(sym_old);
		asm_function *func_new = file2->get_function(sym_new);
		map<string, string> func_symbol_map;

		// TODO: Don't compare local and global symbols
		changed = !compare_functions(file1, func_old,
					     file2, func_new,
					     func_symbol_map);

		delete func_old;
		delete func_new;
	}

	return changed;
}

void diff(asm_file *file1, asm_file *file2, ostream &os)
{
	vector<string> changed_functions;
	vector<string> removed_functions;
	vector<string> new_functions;
	bool changes = false;

	/* Search for new and changed functions */
	for (map<string, asm_function>::const_iterator it = file2->functions.begin();
	     it != file2->functions.end();
	     it++) {
		string name = it->first;
		bool changed;

		if (generated_symbol(name))
			continue;

		if (!file1->has_function(name)) {
			new_functions.push_back(name);
			continue;
		}

		asm_function *func1 = file1->get_function(name);
		asm_function *func2 = file2->get_function(name);

		func1->normalize();
		func2->normalize();

		map<string, string> symbol_map;

		changed = !compare_functions(file1, func1, file2, func2, symbol_map);

		if (!changed)
			changed = compare_symbol_map(file1, file2, symbol_map);

		if (changed)
			changed_functions.push_back(name);

#if 0
		cout << "Compared Function " << *it << endl;
		if (!symbol_map.empty()) {
			cout << "  Symbol map:" << endl;
			for (map<string, string>::iterator sym = symbol_map.begin();
					sym != symbol_map.end();
					++sym) {
				cout << "    " << sym->first << " = " << sym->second << endl;
			}
		}
#endif

		delete func1;
		delete func2;
		func1 = func2 = 0;
	}

	/* Look for removed functions */
	for (map<string, asm_function>::const_iterator it = file1->functions.begin();
	     it != file1->functions.end();
	     it++) {
		string name = it->first;

		if (generated_symbol(name))
			continue;

		if (!file2->has_function(name))
			removed_functions.push_back(name);
	}

	if (function_list("Removed", removed_functions, os))
		changes = true;

	if (function_list("New",     new_functions,     os))
		changes = true;

	if (function_list("Changed", changed_functions, os))
		changes = true;

	if (!changes)
		os << "No changes found" << endl;
}
