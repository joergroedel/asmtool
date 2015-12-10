#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
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

enum asm_token::token_type asm_token::get_type() const
{
	return type;
}

void asm_token::set_token(string _token)
{
	token = _token;
}

const std::string& asm_token::get_token() const
{
	return token;
}

bool asm_token::operator==(const asm_token& _token) const
{
	return (type == _token.type) && (token == _token.token);
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
		if (it->get_type() != asm_token::IDENTIFIER)
			continue;
		if (it->get_token() != from)
			continue;
		it->set_token(to);
	}
}

bool asm_param::operator==(const asm_param& _param) const
{
	size_t size = tokens.size();

	if (_param.tokens.size() != size)
		return false;

	for (int i = 0; i < size; i++) {
		if (!(tokens[i] == _param.tokens[i]))
			return false;
	}

	return true;
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

bool asm_instruction::operator==(const asm_instruction& _instruction) const
{
	size_t size = params.size();

	if (instruction != _instruction.instruction)
		return false;

	if (_instruction.params.size() != size)
		return false;

	for (int i = 0; i < size; i++) {
		if (!(params[i] == _instruction.params[i]))
			return false;
	}

	return true;
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

void asm_label::set_label(string _label)
{
	label = _label;
}

bool asm_label::operator==(const asm_label& _label) const
{
	return label == _label.label;
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

#if 0
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

void statement::rename_label(std::string from, std::string to)
{
	if (_type == LABEL && c_label->get_label() == from)
		c_label->set_label(to);
	else if (_type == INSTRUCTION)
		c_instruction->rename_label(from, to);
}

bool statement::operator==(const statement& _statement) const
{
	if (_type != _statement._type)
		return false;

	if (_type == INSTRUCTION)
		return *c_instruction == *_statement.c_instruction;
	else if (_type == LABEL)
		return *c_label == *_statement.c_label;
	else
		return false;
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

asm_function::asm_function(const string& _name)
	: name(_name)
{
}

void asm_function::add_statement(const statement& stmt)
{
	statements.push_back(stmt);
}

void asm_function::normalize()
{
	map<string, string> symbols;
	ostringstream is;
	int counter = 0;

	/* Create replacements for function-local labels */
	for (vector<statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++)
	{
		asm_label *label;

		if (it->type() != LABEL)
			continue;

		is << ".ADL" << counter;
		label = it->obj_label();

		symbols[label->get_label()] = is.str();
		is.clear();
		counter += 1;
	}

	/* Rename labels */
	for (vector<statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++) {
		for (map<string, string>::iterator sym = symbols.begin();
		     sym != symbols.end();
		     sym++)
			it->rename_label(sym->first, sym->second);
	}
}

bool asm_function::operator==(const asm_function &func) const
{
	size_t size = statements.size();

	if (name != func.name)
		return false;

	if (size != func.statements.size())
		return false;

	for (int i = 0; i < size; i++) {
		if (!(statements[i] == func.statements[i]))
			return false;
	}

	return true;
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

asm_function *asmfile::get_function(string name)
{
	bool in_text = false, old_text = false;
	asm_function *func = 0;
	int depth = 0;

	for (vector<statement>::iterator it = statements.begin();
	     it != statements.end();
	     it++) {
		stmt_type type = it->type();

		/* Section tracking */
		if (type == TEXT) {
			in_text = true;
			depth   = 0;
		} else if (type == DATA) {
			in_text = false;
		} else if (type == PUSHSECTION) {
			depth += 1;
			old_text = in_text;
		} else if (type == POPSECTION) {
			depth -= 1;
			if (depth == 0)
				in_text = old_text;
		} else if (type == SECTION) {
			in_text = false;
		}

		/* Search for the function */
		if (!func) {
			if (type != LABEL)
				continue;

			if (it->obj_label()->get_label() != name)
				continue;

			func = new asm_function(name);

			continue;
		}

		/* We found the function */

		if (!in_text)
			continue;

		if (type == INSTRUCTION || type == LABEL)
			func->add_statement(*it);

		/* HACK: use better check for eofunc */
		if (type == SIZE)
			break;

	}

	return func;
}
