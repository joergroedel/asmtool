#ifndef __ASMFILE_H
#define __ASMFILE_H

#include <string>
#include <vector>

enum stmt_type {
	NOSTMT = 0,
	DOTFILE,
	STRING,
	INSTRUCTION,
	SECTION,
	TEXT,
	DATA,
	BSS,
	TYPE,
	GLOBAL,
	LOCAL,
	BYTE,
	WORD,
	LONG,
	QUAD,
	ORG,
	ZERO,
	SIZE,
	ALIGN,
	P2ALIGN,
	COMM,
	POPSECTION,
	PUSHSECTION,
	LABEL,
	IDENT,
};

class asm_param {
private:
	std::vector<std::string> tokens;

public:
	void add_token(const std::string& token);
	void reset();
};

class asm_instruction {
private:
	std::string instruction;
	std::string param;
	std::vector<asm_param> params;

public:
	asm_instruction(std::string _instruction, std::string _param);
};

class asm_type {
public:

	enum asm_symbol_type {
		FUNCTION,
		OBJECT,
		UNKNOWN,
	};

private:
	std::string symbol;
	asm_symbol_type type;

	enum asm_symbol_type get_symbol_type(std::string param);

public:
	asm_type(std::string _param);
	const std::string& get_symbol() const;
	enum asm_symbol_type get_symbol_type() const;

};

class asm_label {
private:
	std::string label;

public:
	asm_label(std::string _label);
	const std::string& get_label() const;
};

class statement {
private:
	stmt_type _type;
	std::vector<std::string> params;

	union {
		asm_instruction	*c_instruction;
		asm_type	*c_type;
		asm_label	*c_label;
	};

public:
	statement(const std::string &line);
	statement(const statement& stmt);
	~statement();
	stmt_type type() const;

	asm_instruction	*obj_intruction();
	asm_type	*obj_type();
	asm_label	*obj_label();
};

class asmfile {
private:
	std::vector<statement>		statements;
	std::vector<std::string>	functions;
	std::vector<std::string>	objects;

public:
	asmfile();
	void add_statement(const statement &stmt);
	void analyze();
};

#endif
