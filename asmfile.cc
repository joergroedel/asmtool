#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

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

	while (true) {
		asm_param p;

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

asm_instruction::asm_instruction(std::string _instruction, std::string _param)
	: instruction(_instruction), param(_param)
{
	params = parse_asm_params(param);
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

const std::string& asm_type::get_symbol() const
{
	return symbol;
}

enum asm_type::asm_symbol_type asm_type::get_symbol_type() const
{
	return type;
}

asm_label::asm_label(std::string _label)
	: label(_label)
{
}

const std::string& asm_label::get_label() const
{
	return label;
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

statement::statement(const std::string &line)
	: c_instruction(0)
{
	string first, second;
	const stmt_map *map;
	int i = 0;

	vector<string> items = split_trim(" \t", line, 1);
	if (items.size() == 0)
		return;

	_type = NOSTMT;

	for (map = __stmt_map; map->type != NOSTMT; map++) {
		if (items[0] == map->str) {
			_type = map->type;
			i = 1;
			break;
		}
	}

	if (_type == NOSTMT) {
		if (items[0].at(items[0].size()-1) == ':') {
			_type = LABEL;
			items[0].erase(items[0].end() - 1);
		} else if (items[0][0] == '.') {
			cout << "Unknown item: " << items[0] << endl;
			return;
		} else {
			_type = INSTRUCTION;
		}
	}

	for (; i < items.size(); i++)
		params.push_back(items[i]);

	if (params.size() > 0)
		first = params[0];
	if (params.size() > 1)
		second = params[1];

	if (_type == INSTRUCTION)
		c_instruction = new asm_instruction(first, second);
	else if (_type == TYPE)
		c_type = new asm_type(first);
	else if (_type == LABEL)
		c_label = new asm_label(first);

#if 1
	cout << __stmt_name[_type] << " ";
	for (vector<string>::iterator it =  params.begin(); it != params.end(); it++)
		cout << '[' << *it << "] ";
	cout << endl;
#endif
}

statement::statement(const statement& stmt)
	: c_instruction(0)
{
	_type = stmt._type;

	if (_type == INSTRUCTION && stmt.c_instruction)
		c_instruction = new asm_instruction(*stmt.c_instruction);
	else if (_type == TYPE && stmt.c_type)
		c_type = new asm_type(*stmt.c_type);
	else if (_type == LABEL && stmt.c_label)
		c_label = new asm_label(*stmt.c_label);
}

statement::~statement()
{
	if (_type == INSTRUCTION && c_instruction)
		delete c_instruction;
	else if (_type == TYPE && c_type)
		delete c_type;
	else if (_type == LABEL && c_label)
		delete c_label;
}

stmt_type statement::type() const
{
	return _type;
}

asm_instruction *statement::obj_intruction()
{
	return c_instruction;
}

asm_type *statement::obj_type()
{
	return c_type;
}

asm_label *statement::obj_label()
{
	return c_label;
}

asmfile::asmfile()
	: statements()
{
}

void asmfile::add_statement(const statement &stmt)
{
	statements.push_back(stmt);
}

void asmfile::analyze()
{
	asm_type *c_type;

	for (vector<statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++) {
		if (it->type() != TYPE)
			continue;

		c_type = it->obj_type();

		if (c_type->get_symbol_type() == asm_type::FUNCTION)
			functions.push_back(c_type->get_symbol());
		else if (c_type->get_symbol_type() == asm_type::OBJECT)
			objects.push_back(c_type->get_symbol());
		else
			cerr << "Unknown type " << c_type->get_symbol() << endl;
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

