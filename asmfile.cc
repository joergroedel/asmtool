/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>

#include "asmfile.h"
#include "helper.h"

using namespace std;

static vector<asm_param> parse_asm_params(string param)
{
	enum asm_token::token_type type = asm_token::UNKNOWN;
	vector<asm_param> params;
	size_t size, len = 0;
	int depth = 0;
	string token;
	asm_param p;

	while (true) {

		param = trim(param);
		size  = param.size();

		if (size == 0)
			break;

		switch (param[0]) {
		/* Operators */
		case ',':
			if (depth == 0) {
				params.push_back(p);
				p.reset();
				param = param.substr(1);
			} else {
				len  = 1;
				type = asm_token::OPERATOR;

			}
			break;
		case '+':
		case '-':
		case '*':
		case '/':
			len  = 1;
			type = asm_token::OPERATOR;
			break;
		case '(':
			depth += 1;
			len    = 1;
			type   = asm_token::OPERATOR;
			break;
		case ')':
			depth -= 1;
			len    = 1;
			type   = asm_token::OPERATOR;
			break;
		case '.':
		case '_':
		case 'A' ... 'Z':
		case 'a' ... 'z':
			/* Identifier */
			for (len = 1; len < size; len++) {
				char a = param[len];
				if (!((a == '.') || (a == '_') ||
				      (a >= 'a' && a <= 'z') ||
				      (a >= 'A' && a <= 'Z') ||
				      (a >= '0' && a <= '9')))
					break;
			}
			type = asm_token::IDENTIFIER;
			break;
		case '%':
			/* Register */
			for (len = 1; len < size; len++) {
				char a = param[len];
				if (!((a >= 'a' && a <= 'z') ||
				      (a >= 'A' && a <= 'Z') ||
				      (a >= '0' && a <= '9')))
					break;
			}
			type = asm_token::REGISTER;
			break;
		case '$':
			/* Constant number */
			for (len = 1; len < size; len++) {
				char a = param[len];
				if (!((len == 2 && (a == 'x' || a == 'X')) ||
				      (a >= 'a' && a <= 'f') ||
				      (a >= 'A' && a <= 'F') ||
				      (a >= '0' && a <= '9')))
					break;
			}
			type = asm_token::NUMBER;
			break;
		case '0' ... '9':
			/* Offset numbers */
			for (len = 1; len < size; len++) {
				char a = param[len];
				if (!((len == 1 && (a == 'x' || a == 'X')) ||
				      (a >= 'a' && a <= 'f') ||
				      (a >= 'A' && a <= 'F') ||
				      (a >= '0' && a <= '9')))
					break;
			}
			type = asm_token::NUMBER;
			break;
		}

		if (len) {
			token = param.substr(0, len);
			p.add_token(asm_token(token, type));
			param = param.substr(len);
			if (len == size)
				params.push_back(p);
			len = 0;
		}
	}

	return params;
}

asm_token::asm_token(const string& _token, enum asm_token::token_type _type)
	: token(_token), type(_type)
{
}

void asm_param::add_token(const asm_token& token)
{
	tokens.push_back(token);
}

void asm_param::reset()
{
	tokens.clear();
}

void asm_param::rename_label(string from, string to)
{
	for (vector<asm_token>::iterator it = tokens.begin();
	     it != tokens.end();
	     it++) {
		if (it->type != asm_token::IDENTIFIER)
			continue;
		if (it->token != from)
			continue;
		it->token = to;
	}
}

asm_instruction::asm_instruction(std::string _instruction, std::string _param)
	: instruction(_instruction), param(_param)
{
	params = parse_asm_params(param);
}

void asm_instruction::rename_label(string from, string to)
{
	for (vector<asm_param>::iterator it = params.begin();
	     it != params.end();
	     it++)
		it->rename_label(from, to);
}

enum asm_type::asm_symbol_type asm_type::get_symbol_type(string param)
{
	if (param == "@function")
		return asm_type::FUNCTION;
	else if (param == "@object")
		return asm_type::OBJECT;
	else
		return asm_type::UNKNOWN;
}

asm_type::asm_type(std::string _param)
{
	vector<string> items = split_trim(",", _param, 1);

	if (items.size() > 0)
		symbol = items[0];
	if (items.size() > 1)
		type = get_symbol_type(items[1]);
}

asm_label::asm_label(std::string _label)
	: label(_label)
{
}

asm_size::asm_size(string param)
{
	vector<string> items = split_trim(",", param);

	if (items.size() > 0)
		symbol = items[0];
	if (items.size() > 1)
		size   = items[1];
}

asm_section::asm_section(string param)
{
	vector<string> items = split_trim(",", param, 2);

	if (items.size() > 0)
		name   = items[0];
	if (items.size() > 1)
		flags  = items[1];
	if (items.size() > 2)
		params = items[2];

	executable = (flags.find_first_of("x") != string::npos);
}

asm_section::asm_section()
	: name(".text"), flags(), params(), executable(true)
{
}


const struct stmt_map {
	const char *str;
	stmt_type type;
} __stmt_map[] = {
	{ .str = ".file",	.type = DOTFILE		},
	{ .str = ".string",	.type = STRING		},
	{ .str = ".section",	.type = SECTION		},
	{ .str = ".text",	.type = TEXT		},
	{ .str = ".data",	.type = DATA		},
	{ .str = ".bss",	.type = BSS		},
	{ .str = ".type",	.type = TYPE		},
	{ .str = ".globl",	.type = GLOBAL		},
	{ .str = ".local",	.type = LOCAL		},
	{ .str = ".byte",	.type = BYTE		},
	{ .str = ".word",	.type = WORD		},
	{ .str = ".long",	.type = LONG		},
	{ .str = ".quad",	.type = QUAD		},
	{ .str = ".org",	.type = ORG		},
	{ .str = ".zero",	.type = ZERO		},
	{ .str = ".size",	.type = SIZE		},
	{ .str = ".align",	.type = ALIGN		},
	{ .str = ".p2align",	.type = P2ALIGN		},
	{ .str = ".comm",	.type = COMM		},
	{ .str = ".popsection",	.type = POPSECTION	},
	{ .str = ".pushsection",.type = PUSHSECTION	},
	{ .str = ".ident",	.type = IDENT		},
	{ .str = 0,		.type = NOSTMT		},
};

const char *__stmt_name[] = {
	[NOSTMT]	= "NO STATEMENT",
	[DOTFILE]	= "FILE",
	[STRING]	= "STRING",
	[INSTRUCTION]	= "INSTRUCTION",
	[SECTION]	= "SECTION",
	[TEXT]		= "TEXT",
	[DATA]		= "DATA",
	[BSS]		= "BSS",
	[TYPE]		= "TYPE",
	[GLOBAL]	= "GLOBAL",
	[LOCAL]		= "LOCAL",
	[BYTE]		= "BYTE",
	[WORD]		= "WORD",
	[LONG]		= "LONG",
	[QUAD]		= "QUAD",
	[ORG]		= "ORG",
	[ZERO]		= "ZERO",
	[SIZE]		= "SIZE",
	[ALIGN]		= "ALIGN",
	[P2ALIGN]	= "P2ALIGN",
	[COMM]		= "COMM",
	[POPSECTION]	= "POPSECTION",
	[PUSHSECTION]	= "PUSHSECTION",
	[LABEL]		= "LABEL",
	[IDENT]		= "IDENT",
};

asm_statement::asm_statement(const std::string &line)
	: obj_instruction(0)
{
	string first, second;
	const stmt_map *map;
	size_t i = 0;

	vector<string> items = split_trim(" \t", line, 1);
	if (items.size() == 0)
		return;

	type = NOSTMT;

	for (map = __stmt_map; map->type != NOSTMT; map++) {
		if (items[0] == map->str) {
			type = map->type;
			i = 1;
			break;
		}
	}

	if (type == NOSTMT) {
		if (items[0].at(items[0].size()-1) == ':') {
			type = LABEL;
			items[0].erase(items[0].end() - 1);
		} else if (items[0][0] == '.') {
			cout << "Unknown item: " << items[0] << endl;
			return;
		} else {
			type = INSTRUCTION;
		}
	}

	for (; i < items.size(); i++)
		params.push_back(items[i]);

	if (params.size() > 0)
		first = params[0];
	if (params.size() > 1)
		second = params[1];

	if (type == INSTRUCTION)
		obj_instruction = new asm_instruction(first, second);
	else if (type == TYPE)
		obj_type = new asm_type(first);
	else if (type == LABEL)
		obj_label = new asm_label(first);
	else if (type == SIZE)
		obj_size = new asm_size(first);
	else if (type == SECTION)
		obj_section = new asm_section(first);

#if 0
	cout << __stmt_name[type] << " ";
	for (vector<string>::iterator it =  params.begin(); it != params.end(); it++)
		cout << '[' << *it << "] ";
	cout << endl;
#endif
}

asm_statement::asm_statement(const asm_statement& stmt)
	: obj_instruction(0)
{
	type = stmt.type;

	if (type == INSTRUCTION && stmt.obj_instruction)
		obj_instruction = new asm_instruction(*stmt.obj_instruction);
	else if (type == TYPE && stmt.obj_type)
		obj_type = new asm_type(*stmt.obj_type);
	else if (type == LABEL && stmt.obj_label)
		obj_label = new asm_label(*stmt.obj_label);
	else if (type == SIZE && stmt.obj_size)
		obj_size = new asm_size(*stmt.obj_size);
	else if (type == SECTION && stmt.obj_section)
		obj_section = new asm_section(*stmt.obj_section);
}

asm_statement::~asm_statement()
{
	if (type == INSTRUCTION && obj_instruction)
		delete obj_instruction;
	else if (type == TYPE && obj_type)
		delete obj_type;
	else if (type == LABEL && obj_label)
		delete obj_label;
	else if (type == SIZE && obj_size)
		delete obj_size;
	else if (type == SECTION && obj_section)
		delete obj_section;
}

void asm_statement::rename_label(std::string from, std::string to)
{
	if (type == LABEL && obj_label->label == from)
		obj_label->label = to;
	else if (type == INSTRUCTION)
		obj_instruction->rename_label(from, to);
}

asm_function::asm_function(const string& _name)
	: name(_name)
{
}

void asm_function::add_statement(const asm_statement& stmt)
{
	statements.push_back(stmt);
}

void asm_function::normalize()
{
	map<string, string> symbols;
	ostringstream is;
	int counter = 0;

	/* Create replacements for function-local labels */
	for (vector<asm_statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++)
	{
		asm_label *label;

		if (it->type != LABEL)
			continue;

		is << ".ADL" << counter;
		label = it->obj_label;

		symbols[label->label] = is.str();
		is.clear();
		counter += 1;
	}

	/* Rename labels */
	for (vector<asm_statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++) {
		for (map<string, string>::iterator sym = symbols.begin();
		     sym != symbols.end();
		     sym++)
			it->rename_label(sym->first, sym->second);
	}
}

asm_file::asm_file()
	: statements()
{
}

void asm_file::add_statement(const asm_statement &stmt)
{
	statements.push_back(stmt);
}

void asm_file::analyze()
{
	asm_type *obj_type;

	for (vector<asm_statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++) {
		if (it->type != TYPE)
			continue;

		obj_type = it->obj_type;

		if (obj_type->type == asm_type::FUNCTION)
			functions.push_back(obj_type->symbol);
		else if (obj_type->type == asm_type::OBJECT)
			objects.push_back(obj_type->symbol);
		else
			cerr << "Unknown type " << obj_type->symbol << endl;
	}

	sort(functions.begin(), functions.end());
	sort(objects.begin(),   objects.end());

#if 0
	cout << "Functions:" << endl;
	for (vector<string>::iterator it = functions.begin();
	     it != functions.end();
	     it++)
		cout << "    " << *it << endl;

	cout << "Objects:" << endl;
	for (vector<string>::iterator it = objects.begin();
	     it != objects.end();
	     it++)
		cout << "    " << *it << endl;
#endif
}

asm_function *asm_file::get_function(string name)
{
	stack<asm_section> stack;
	asm_function *func = 0;
	asm_section section;

	for (vector<asm_statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++) {
		stmt_type type = it->type;
		asm_section text, data(".data,\"rw\"");

		/* Section tracking */
		if (type == TEXT) {
			section = text;
		} else if (type == DATA) {
			section = data;
		} else if (type == PUSHSECTION) {
			stack.push(section);
		} else if (type == POPSECTION) {
			if (!stack.empty()) {
				section = stack.top();
				stack.pop();
			} else {
				cout << "WARNING: .popsection on empty stack" << endl;
			}
		} else if (type == SECTION) {
			section = *(it->obj_section);
		}

		/* Search for the function */
		if (!func) {
			if (type != LABEL)
				continue;

			if (it->obj_label->label != name)
				continue;

			func = new asm_function(name);

			continue;
		}

		/* We found the function */

		if (!section.executable)
			continue;

		if (type == INSTRUCTION || type == LABEL)
			func->add_statement(*it);

		if (type == SIZE && it->obj_size) {
			if (it->obj_size->symbol == name)
				break;
		}

	}

	func->normalize();

	return func;
}

bool asm_file::has_function(std::string name) const
{
	for (vector<string>::const_iterator it = functions.begin();
	     it != functions.end();
	     it++) {
		if (name == *it)
			return true;
	}

	return false;
}

