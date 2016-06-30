/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>

#include "asmfile.h"
#include "helper.h"
#include "diff.h"

using namespace std;

diff_options::diff_options()
	: show(false), full(false), context(3)
{ }

/* Used in LCS computation and diff creation */
struct __matrix {
	unsigned x, y;
	int *m;
	bool *r;

	__matrix(unsigned _x, unsigned _y)
		: x(_x), y(_y)
	{
		m = new int[_x * _y];
		r = new bool[_x * _y];
	}

	~__matrix()
	{
		delete[] m;
	}

	void set(unsigned _x, unsigned _y, int value)
	{
		m[_y*x+_x] = value;
	}

	void set_r(unsigned _x, unsigned _y, bool  value)
	{
		r[_y*x+_x] = value;
	}

	int get(unsigned _x, unsigned _y)
	{
		return m[_y*x+_x];
	}

	bool get_r(unsigned _x, unsigned _y)
	{
		return r[_y*x+_x];
	}
};

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

#if 0
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
#endif

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

bool compare_statements(asm_file *file1, asm_statement &s1,
			asm_file *file2, asm_statement &s2,
			map<string, string> &symbol_map)
{
	if (s1.type != s2.type)
		return false;

	/*
	 * First check for labels. They need to be identical because
	 * they are normalized between the two versions of the function
	 */
	if ((s1.type == LABEL) &&
	    (s1.obj_label->label != s2.obj_label->label))
		return false;

	if (s1.type == INSTRUCTION) {
		if (!compare_instructions(file1, s1.obj_instruction,
					  file2, s2.obj_instruction,
					  symbol_map))
			return false;
	}

	if (s1.type != LABEL && s1.type != INSTRUCTION) {
		unsigned int i;

		if (s1.params.size() != s2.params.size())
			return false;

		for (i = 0; i < s1.params.size(); i++) {
			if (s1.params[i] != s2.params[i]) {
				return false;
			}
		}
	}

	return true;
}

bool compare_functions(asm_file *file1, asm_function *f1,
		       asm_file *file2, asm_function *f2,
		       map<string, string> &symbol_map,
		       struct __matrix &m)
{
	int size1 = f1->statements.size();
	int size2 = f2->statements.size();

	/*
	 * Compute LCS matrix of both functions to
	 * detect where they are different
	 */
	for(unsigned i = 0; i < m.x; i++)
	{
		for(unsigned j = 0; j < m.y; j++)
		{
			if (i == 0 || j == 0) {
				m.set(i, j, 0);
				continue;
			}

			asm_statement &s1 = f1->statements[i - 1];
			asm_statement &s2 = f2->statements[j - 1];

			if (compare_statements(file1, s1, file2, s2, symbol_map)) {
				m.set(i, j, m.get(i - 1, j - 1) + 1);
				m.set_r(i, j, true);
			} else {
				int a = m.get(i - 1, j);
				int b = m.get(i, j - 1);
				m.set(i, j, max(a, b));
				m.set_r(i, j, false);
			}
		}
	}

	return (size1 == size2 && m.get(size1, size1) == size1);
#if 0
	/* First check if the functions have the same size */
	if (size1 != size2)
		return false;

	/* Now iterate over the function statements */
	for (size_t i = 0; i < size1; ++i) {
		asm_statement &s1 = f1->statements[i];
		asm_statement &s2 = f2->statements[i];

		ret = ret && compare_statements(file1, s1, file2, s2, symbol_map);
	}

	return ret;
#endif
}

static bool function_list(string type, const vector<string> &list, ostream &os)
{
	if (!list.size())
		return false;

	os << type << " functions:" << endl;

	for (vector<string>::const_iterator it = list.begin();
	     it != list.end();
	     it++) {
		os << "    " << *it << endl;
	}

	return true;
}

#if 0
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
#endif

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define WIDTH	48

struct diff_item {
	enum {
		PLUS,
		MINUS,
		EQUAL,
	} change;

	string old_line;
	string new_line;
};

static void print_diff(vector<diff_item> &diff,
		       ostream &os,
		       struct diff_options &opts)
{
	int context = opts.context;
	int size = diff.size();
	int last_printed = -1;

	for (int i = 0; i < size; ++i) {
		int start, end, j;

		if (diff[i].change == diff_item::EQUAL)
			continue;

		if (i - context > last_printed) {
			os << "         [...]" << endl;
			start = i - context;
		} else {
			start = last_printed + 1;
		}

		end = min(i + context, size - 1);


		for (j = start; j < end; ++j) {
			string line = diff[j].old_line;
			const char *color;
			char c = ' ';

			color = BLACK;

			if (diff[j].change == diff_item::PLUS) {
				c = '+';
				color = GREEN;
				line = diff[j].new_line;
				end = min(end + 1, size - 1);
			} else if (diff[j].change == diff_item::MINUS) {
				color = RED;
				c = '-';
				end = min(end + 1, size - 1);
			}

			os << "        " << color << c << line << RESET << endl;

			last_printed = j;
		}

		i = j + 1;
	}
}

static void __create_diff(struct __matrix &m,
			  struct asm_function &f1,
			  struct asm_function &f2,
			  vector<diff_item> &output,
			  int i, int j)
{
	struct diff_item item;

	if (i > 0 && j > 0 && m.get_r(i, j)) {
		string stmt1 = expand_tab(trim(f1.statements[i - 1].stmt));
		string stmt2 = expand_tab(trim(f2.statements[j - 1].stmt));

		__create_diff(m, f1, f2, output, i - 1, j - 1);

		item.change   = diff_item::EQUAL,
		item.old_line = stmt1;
		item.new_line = stmt2;

		output.push_back(item);
	} else if (j > 0 && (i == 0 || m.get(i, j - 1) >= m.get(i - 1, j))) {
		string stmt = expand_tab(trim(f2.statements[j - 1].stmt));

		__create_diff(m, f1, f2, output, i, j - 1);

		item.change   = diff_item::PLUS;
		item.old_line = "";
		item.new_line = stmt;

		output.push_back(item);
	} else if (i > 0 && (j == 0 || m.get(i, j - 1) < m.get(i - 1, j))) {
		string stmt = expand_tab(trim(f1.statements[i - 1].stmt));

		__create_diff(m, f1, f2, output, i - 1, j);

		item.change   = diff_item::MINUS;
		item.old_line = stmt;
		item.new_line = "";

		output.push_back(item);
	}
}

static void create_diff(struct __matrix &m,
			struct asm_function &f1,
			struct asm_function &f2,
			vector<diff_item> &output)
{
	__create_diff(m, f1, f2, output, m.x - 1, m.y - 1);
}

struct changed_function {
	string name;
	vector<diff_item> diff;
};

void diff(asm_file *file1, asm_file *file2, ostream &os, struct diff_options &opts)
{
	vector<changed_function> changed_functions;
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

		int size1 = func1->statements.size();
		int size2 = func2->statements.size();
		struct __matrix m(size1 + 1, size2 + 1);

		changed = !compare_functions(file1, func1, file2, func2, symbol_map, m);

#if 0
		if (!changed)
			changed = compare_symbol_map(file1, file2, symbol_map);
#endif

		if (changed) {
			struct changed_function cf;
			size_t i;

			cf.name = name;
			changed_functions.push_back(cf);

			i = changed_functions.size() - 1;

			create_diff(m, *func1, *func2, changed_functions[i].diff);
		}

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

	if (changed_functions.size() > 0) {
		changes = true;

		os << "Changed functions:" << endl;

		for (vector<changed_function>::iterator i = changed_functions.begin();
		     i != changed_functions.end();
		     ++i) {
			os << "    " << i->name << ':' << endl;
			if (opts.show)
				print_diff(i->diff, os, opts);
		     }
	}

	if (!changes)
		os << "No changes found" << endl;
}
