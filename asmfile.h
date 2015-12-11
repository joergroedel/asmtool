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

struct asm_token {
public:
	enum token_type {
		UNKNOWN,
		OPERATOR,
		IDENTIFIER,
		REGISTER,
		NUMBER,
	};

	std::string token;
	token_type type;

	asm_token(const std::string& _token, enum token_type _type);
};

struct asm_param {

	std::vector<asm_token> tokens;

	void add_token(const asm_token& token);
	void reset();
	void rename_label(std::string from, std::string to);
};

struct asm_instruction {

	std::string instruction;
	std::string param;
	std::vector<asm_param> params;

	asm_instruction(std::string _instruction, std::string _param);
	void rename_label(std::string from, std::string to);
};

struct asm_type {

	enum asm_symbol_type {
		FUNCTION,
		OBJECT,
		UNKNOWN,
	};

	std::string symbol;
	asm_symbol_type type;

	enum asm_symbol_type get_symbol_type(std::string param);

	asm_type(std::string _param);
};

struct asm_label {

	std::string label;

	asm_label(std::string _label);
};

struct asm_statement {

	stmt_type type;
	std::vector<std::string> params;

	union {
		asm_instruction	*obj_instruction;
		asm_type	*obj_type;
		asm_label	*obj_label;
	};

	asm_statement(const std::string &line);
	asm_statement(const asm_statement& stmt);
	~asm_statement();
	void rename_label(std::string from, std::string to);
};

struct asm_function {

	std::string name;
	std::vector<asm_statement> statements;

	asm_function(const std::string& name);
	void add_statement(const asm_statement &stmt);

	void normalize();
};

struct asm_file {

	std::vector<asm_statement>	statements;
	std::vector<std::string>	functions;
	std::vector<std::string>	objects;

	asm_file();
	void add_statement(const asm_statement &stmt);
	void analyze();

	asm_function *get_function(std::string name);
	bool has_function(std::string name) const;
};

#endif
