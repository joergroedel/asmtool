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

class asm_token {
public:
	enum token_type {
		UNKNOWN,
		OPERATOR,
		IDENTIFIER,
		REGISTER,
		NUMBER,
	};

	asm_token(const std::string& _token, enum token_type _type);
	token_type get_type() const;
	void set_token(std::string _token);
	const std::string& get_token() const;

	bool operator==(const asm_token& _token) const;

private:
	std::string token;
	token_type type;
};

class asm_param {
private:
	std::vector<asm_token> tokens;

public:
	void add_token(const asm_token& token);
	void reset();
	void rename_label(std::string from, std::string to);

	bool operator==(const asm_param& _param) const;
};

class asm_instruction {
private:
	std::string instruction;
	std::string param;
	std::vector<asm_param> params;

public:
	asm_instruction(std::string _instruction, std::string _param);
	void rename_label(std::string from, std::string to);

	bool operator==(const asm_instruction& _instruction) const;
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
	void set_label(std::string _label);

	bool operator==(const asm_label& _label) const;
};

class asm_statement {
private:
	stmt_type _type;
	std::vector<std::string> params;

	union {
		asm_instruction	*c_instruction;
		asm_type	*c_type;
		asm_label	*c_label;
	};

public:
	asm_statement(const std::string &line);
	asm_statement(const asm_statement& stmt);
	~asm_statement();
	stmt_type type() const;
	void rename_label(std::string from, std::string to);

	bool operator==(const asm_statement& _statement) const;

	asm_instruction	*obj_intruction();
	asm_type	*obj_type();
	asm_label	*obj_label();
};

class asm_function {
private:
	std::string name;
	std::vector<asm_statement> statements;

public:
	asm_function(const std::string& name);
	void add_statement(const asm_statement &stmt);

	void normalize();

	bool operator==(const asm_function &func) const;
};

class asmfile {
private:
	std::vector<asm_statement>		statements;
	std::vector<std::string>	functions;
	std::vector<std::string>	objects;

public:
	asmfile();
	void add_statement(const asm_statement &stmt);
	void analyze();

	asm_function *get_function(std::string name);
	bool has_function(std::string name) const;

	std::vector<std::string>::const_iterator functions_begin() const;
	std::vector<std::string>::const_iterator functions_end() const;
};

#endif
