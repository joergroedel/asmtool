#include <algorithm>
#include <sstream>

#include <ctype.h>

#include "newdata.h"
#include "helper.h"

namespace assembly {

	static const struct __stmt_map {
		const char *str;
		stmt_type type;
	} stmt_map[] = {
		{ .str = ".file",			.type = stmt_type::DOTFILE		},
		{ .str = ".section",			.type = stmt_type::SECTION		},
		{ .str = ".text",			.type = stmt_type::TEXT			},
		{ .str = ".data",			.type = stmt_type::DATA			},
		{ .str = ".bss",			.type = stmt_type::BSS			},
		{ .str = ".type",			.type = stmt_type::TYPE			},
		{ .str = ".globl",			.type = stmt_type::GLOBAL		},
		{ .str = ".local",			.type = stmt_type::LOCAL		},
		{ .str = ".string",			.type = stmt_type::STRING		},
		{ .str = ".ascii",			.type = stmt_type::ASCII		},
		{ .str = ".byte",			.type = stmt_type::BYTE			},
		{ .str = ".short",			.type = stmt_type::SHORT		},
		{ .str = ".word",			.type = stmt_type::WORD			},
		{ .str = ".long",			.type = stmt_type::LONG			},
		{ .str = ".quad",			.type = stmt_type::QUAD			},
		{ .str = ".float",			.type = stmt_type::FLOAT		},
		{ .str = ".double",			.type = stmt_type::DOUBLE		},
		{ .str = ".org",			.type = stmt_type::ORG			},
		{ .str = ".zero",			.type = stmt_type::ZERO			},
		{ .str = ".size",			.type = stmt_type::SIZE			},
		{ .str = ".align",			.type = stmt_type::ALIGN		},
		{ .str = ".p2align",			.type = stmt_type::P2ALIGN		},
		{ .str = ".comm",			.type = stmt_type::COMM			},
		{ .str = ".lcomm",			.type = stmt_type::LCOMM		},
		{ .str = ".popsection",			.type = stmt_type::POPSECTION		},
		{ .str = ".pushsection",		.type = stmt_type::PUSHSECTION		},
		{ .str = ".ident",			.type = stmt_type::IDENT		},
		{ .str = ".loc",			.type = stmt_type::LOC			},
		{ .str = ".cfi_startproc",		.type = stmt_type::CFI_STARTPROC	},
		{ .str = ".cfi_endproc",		.type = stmt_type::CFI_ENDPROC		},
		{ .str = ".cfi_offset",			.type = stmt_type::CFI_OFFSET		},
		{ .str = ".cfi_remember_state",		.type = stmt_type::CFI_REMEBER_STATE	},
		{ .str = ".cfi_restore_state",		.type = stmt_type::CFI_RESTORE_STATE	},
		{ .str = ".cfi_restore",		.type = stmt_type::CFI_RESTORE		},
		{ .str = ".cfi_def_cfa_offset",		.type = stmt_type::CFI_DEF_CFA_OFFSET	},
		{ .str = ".cfi_def_cfa_register",	.type = stmt_type::CFI_DEF_CFA_REGISTER	},
		{ .str = ".cfi_def_cfa",		.type = stmt_type::CFI_DEF_CFA		},
		{ .str = ".cfi_sections",		.type = stmt_type::CFI_SECTIONS		},
		{ .str = ".cfi_escape",			.type = stmt_type::CFI_ESCAPE		},
		{ .str = ".balign",			.type = stmt_type::BALIGN		},
		{ .str = ".weak",			.type = stmt_type::WEAK			},
		{ .str = ".value",			.type = stmt_type::VALUE		},
		{ .str = ".uleb128",			.type = stmt_type::ULEB128		},
		{ .str = ".sleb128",			.type = stmt_type::SLEB128		},
		{ .str = 0,				.type = stmt_type::NOSTMT		},
	};

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_token
	//
	/////////////////////////////////////////////////////////////////////

	asm_token::asm_token(std::string token, enum token_type type)
		: m_token(token), m_type(type)
	{
	}

	std::string asm_token::serialize() const
	{
		std::ostringstream os;

		os << '\'';
		if (m_type == token_type::STRING)
			os << '"';

		os << m_token;

		if (m_type == token_type::STRING)
			os << '"';

		os << '\'';

		return os.str();
	}

	enum token_type asm_token::type() const
	{
		return m_type;
	}

	std::string asm_token::token() const
	{
		return m_token;
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_param
	//
	/////////////////////////////////////////////////////////////////////

	template<typename T>
	void asm_param::add_token(T&& t)
	{
		m_tokens.push_back(std::forward<T>(t));
	}

	void asm_param::reset()
	{
		m_tokens.clear();
	}

	size_t asm_param::tokens() const
	{
		return m_tokens.size();
	}

	void asm_param::token(asm_param::token_t::size_type idx,
			      std::function<void(enum token_type, std::string)> handler) const
	{
		if (m_tokens.size() <= idx)
			return;

		handler(m_tokens[idx].type(), m_tokens[idx].token());
	}

	std::string asm_param::serialize() const
	{
		std::ostringstream os;
		int count = 0;

		for_each(m_tokens.begin(), m_tokens.end(), [&](const asm_token& t) {
				if (count++)
					os << ' ';
				os << t.serialize();
				});

		return os.str();
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_statement
	//
	/////////////////////////////////////////////////////////////////////

	asm_statement::asm_statement(std::string stmt)
		: m_stmt(std::move(stmt)), m_type(stmt_type::NOSTMT)
	{
	}

	void asm_statement::rename_label(std::string from, std::string to)
	{
		// TODO
	}

	void asm_statement::analyze()
	{
	}

	void asm_statement::type(enum stmt_type t)
	{
		m_type = t;
	}

	enum stmt_type asm_statement::type() const
	{
		return m_type;
	}

	void asm_statement::set_instr(std::string instr)
	{
		m_instr = instr;
	}

	template<typename T>
	void asm_statement::add_param(T&& p)
	{
		m_params.push_back(std::forward<T>(p));
	}

	std::string asm_statement::serialize() const
	{
		std::ostringstream os;
		int count = 0;

		os << m_instr;

		if (m_type == stmt_type::LABEL)
			os << ':';
		else
			os << ' ';

		for_each (m_params.begin(), m_params.end(), [&](const asm_param& p) {
				if (count++)
					os << ',';
				os << p.serialize();
				});

		return os.str();
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_type
	//
	/////////////////////////////////////////////////////////////////////

	asm_type::asm_type(std::string stmt)
		: asm_statement(std::move(stmt))
	{
		m_type = stmt_type::TYPE;
	}


	enum symbol_type asm_type::get_type() const
	{
		return m_stype;
	}

	std::string asm_type::get_symbol() const
	{
		return m_symbol;
	}

	void asm_type::analyze()
	{
		if (m_params.size() < 2)
			return;

		m_params[0].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::IDENTIFIER)
					m_symbol = token;
				});

		m_params[1].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::TYPEFLAG) {
					if (token == "@function")
						m_stype = symbol_type::FUNCTION;
					else if (token == "@object")
						m_stype = symbol_type::OBJECT;
					else
						m_stype = symbol_type::UNKNOWN;
				}
				});
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_label
	//
	/////////////////////////////////////////////////////////////////////

	asm_label::asm_label(std::string stmt)
		: asm_statement(stmt)
	{
		m_type = stmt_type::LABEL;
	}

	void asm_label::rename_label(std::string from, std::string to)
	{
		asm_statement::rename_label(from, to);

		if (m_instr == from)
			m_instr = to;
	}

	std::string asm_label::get_label() const
	{
		return m_instr;
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_size
	//
	/////////////////////////////////////////////////////////////////////

	asm_size::asm_size(std::string stmt)
		: asm_statement(stmt)
	{
		m_type = stmt_type::SIZE;
	}

	void asm_size::analyze()
	{
		if (m_params.size() < 2)
			return;

		m_params[0].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::IDENTIFIER)
					m_symbol = token;
				});
	}

	std::string asm_size::get_symbol() const
	{
		return m_symbol;
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_section
	//
	/////////////////////////////////////////////////////////////////////

	asm_section::asm_section(std::string stmt)
		: asm_statement(stmt)
	{
		m_type = stmt_type::SECTION;
	}

	void asm_section::analyze()
	{
		auto size = m_params.size();

		if (size < 1)
			return;

		m_params[0].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::IDENTIFIER)
					m_name = token;
				});

		if (size < 2)
			return;

		m_params[1].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::STRING)
					m_flags = token;
				});

		m_executable = (m_flags.find_first_of("x") != std::string::npos);
	}

	std::string asm_section::get_name() const
	{
		return m_name;
	}

	bool asm_section::executable() const
	{
		return m_executable;
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Class asm_comm
	//
	/////////////////////////////////////////////////////////////////////

	asm_comm::asm_comm(std::string stmt)
		: asm_statement(stmt), m_symbol(), m_alignment(0), m_size(0)
	{
		m_type = stmt_type::COMM;
	}

	void asm_comm::analyze()
	{
		auto size = m_params.size();

		if (size < 1)
			return;

		m_params[0].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::IDENTIFIER)
					m_symbol = token;
				});

		if (size < 2)
			return;

		m_params[1].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::NUMBER) {
					std::istringstream is(token);

					is >> m_size;
				}
				});

		if (size < 3)
			return;

		m_params[2].token(0, [&](enum token_type type, std::string token) {
				if (type == token_type::NUMBER) {
					std::istringstream is(token);

					is >> m_alignment;
				}
				});
	}

	std::string asm_comm::get_symbol() const
	{
		return m_symbol;
	}

	/////////////////////////////////////////////////////////////////////
	//
	// Statement parser and helper functions
	//
	/////////////////////////////////////////////////////////////////////

	static bool is_identifier_char(char c)
	{
		return (c == '.' || c == '_'  || isalnum(c));
	}

	static bool is_register_char(char c)
	{
		return (c == '%' || isalnum(c));
	}

	static bool is_typeflag_char(char c)
	{
		return (c == '@' || isalnum(c));
	}

	static bool is_number_char(char c)
	{
		return (c == 'x' || c == 'X' || isxdigit(c));
	}

	std::unique_ptr<asm_statement> parse_statement(std::string stmt)
	{
		std::unique_ptr<asm_statement> statement;
		enum stmt_type stmt_t = stmt_type::NOSTMT;
		std::string instr, params;

		{
			std::vector<std::string> items(split_trim(" \t", stmt, 1));
			auto size = items.size();

			if (size == 0)
				return statement;

			instr = items[0];

			if (size > 1)
				params = items[1];

			// Destroy items now
		}

		// Sanity check
		if (instr.size() == 0)
			return statement;

		// Find out the type of the statement now

		// First check against the map of known statements, if there is
		// no match it could be a lable or an instruction
		for (auto map = stmt_map; map->type != stmt_type::NOSTMT; map++) {
			if (instr == map->str) {
				stmt_t = map->type;
				break;
			}
		}

		// Now check for instructions and labels, in case we don't know
		// the type yet
		if (stmt_t == stmt_type::NOSTMT) {
			auto last = instr.rbegin();

			if (*last == ':') {
				instr.pop_back();
				stmt_t = stmt_type::LABEL;
			} else if (instr[0] == '.') {
				stmt_t = stmt_type::UNKNOWN;
			} else {
				stmt_t = stmt_type::INSTRUCTION;
			}
		}

		switch (stmt_t) {
		case stmt_type::TYPE:
			statement = std::unique_ptr<asm_statement>(new asm_type(stmt));
			break;
		case stmt_type::LABEL:
			statement = std::unique_ptr<asm_statement>(new asm_label(stmt));
			break;
		case stmt_type::SIZE:
			statement = std::unique_ptr<asm_statement>(new asm_size(stmt));
			break;
		case stmt_type::SECTION:
			statement = std::unique_ptr<asm_statement>(new asm_section(stmt));
			break;
		case stmt_type::COMM:
			statement = std::unique_ptr<asm_statement>(new asm_comm(stmt));
			break;
		default:
			statement = std::unique_ptr<asm_statement>(new asm_statement(stmt));
			break;
		}

		statement->type(stmt_t);
		statement->set_instr(instr);

		// Now parse the params, if any
		enum token_type type = token_type::UNKNOWN;
		char last_char = 0;
		std::string token;
		asm_param param;
		int depth = 0;

		for (auto it = params.begin(), end = params.end(); it != end; ++it) {

			if ((type == token_type::IDENTIFIER && is_identifier_char(*it)) ||
			    (type == token_type::REGISTER   && is_register_char(*it))   ||
			    (type == token_type::TYPEFLAG   && is_typeflag_char(*it))   ||
			    (type == token_type::NUMBER     && is_number_char(*it))) {
				token += *it;
				continue;
			} else if (type == token_type::STRING) {
				if (*it == '"' && last_char != '\\') {
					param.add_token(asm_token(token, type));
					token     = "";
					type      = token_type::UNKNOWN;
					last_char = 0;
				} else {
					// guard against '\\"'
					last_char  = last_char != '\\' ? *it : 0;
					token     += *it;
				}
				continue;
			} else if (type == token_type::IDENTIFIER ||
				   type == token_type::REGISTER   ||
				   type == token_type::TYPEFLAG   ||
				   type == token_type::NUMBER) {
				param.add_token(asm_token(token, type));
				token     = "";
				type      = token_type::UNKNOWN;
				last_char = 0;
			}

			// token_type is UNKNOWN
			switch (*it) {
			case ' ':
			case '\t':
				continue;
			case ',':
				if (depth > 0) {
					param.add_token(asm_token(",", token_type::OPERATOR));
					continue;
				}

				statement->add_param(param);
				param.reset();

				break;
			case '(':
			case ')':
				depth += (*it == '(') ? 1 : -1;
				// Fall-Through
			case '+':
			case '-':
			case '*':
			case '/':
			case ':':
				token += *it;
				param.add_token(asm_token(token, token_type::OPERATOR));
				token = "";
				break;
			case '.':
			case '_':
			case 'A' ... 'Z':
			case 'a' ... 'z':
				// Identifier
				type   = token_type::IDENTIFIER;
				token += *it;
				break;
			case '%':
				// Register
				type   = token_type::REGISTER;
				token += *it;
				break;
			case '$':
			case '0' ... '9':
				// Number
				type   = token_type::NUMBER;
				token += *it;
				break;
			case '"':
				type   = token_type::STRING;
				break;
			case '@':
				type   = token_type::TYPEFLAG;
				token += *it;
				break;
			}
		}

		if (type != token_type::UNKNOWN)
			param.add_token(asm_token(token, type));

		if (param.tokens() > 0)
			statement->add_param(param);

		statement->analyze();

		return statement;
	}
}
