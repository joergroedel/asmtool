#ifndef __NEWDATA_H

#include <vector>
#include <string>
#include <memory>

namespace assembly {

	enum class stmt_type {
		NOSTMT = 0,
		UNKNOWN,
		DOTFILE,
		INSTRUCTION,
		SECTION,
		TEXT,
		DATA,
		BSS,
		TYPE,
		GLOBAL,
		LOCAL,
		STRING,
		ASCII,
		BYTE,
		SHORT,
		WORD,
		LONG,
		QUAD,
		FLOAT,
		DOUBLE,
		ORG,
		ZERO,
		SIZE,
		ALIGN,
		P2ALIGN,
		COMM,
		LCOMM,
		POPSECTION,
		PUSHSECTION,
		LABEL,
		IDENT,
		LOC,
		CFI_STARTPROC,
		CFI_ENDPROC,
		CFI_OFFSET,
		CFI_REMEBER_STATE,
		CFI_RESTORE_STATE,
		CFI_RESTORE,
		CFI_DEF_CFA_OFFSET,
		CFI_DEF_CFA_REGISTER,
		CFI_DEF_CFA,
		CFI_SECTIONS,
		CFI_ESCAPE,
		BALIGN,
		WEAK,
		VALUE,
		ULEB128,
		SLEB128,
	};

	enum class token_type {
		UNKNOWN,
		OPERATOR,
		IDENTIFIER,
		REGISTER,
		NUMBER,
		STRING,
		TYPEFLAG,
	};

	class asm_token {
	protected:
		std::string	m_token;
		enum token_type	m_type;

	public:

		asm_token(std::string, enum token_type);
		enum token_type type() const;
		std::string token() const;

		std::string serialize() const;
	};

	class asm_param {
	protected:
		std::vector<asm_token> m_tokens;

	public:
		template<typename T>
		void add_token(T&&);
		void reset();
		size_t tokens() const;
		std::string serialize() const;
	};

	class asm_statement {
		std::string		m_stmt;
		std::string		m_instr;
		enum stmt_type		m_type;
		std::vector<asm_param>	m_params;

	public:
		asm_statement(std::string);
		virtual void rename_label(std::string, std::string);
		virtual void analyze();

		void type(enum stmt_type);
		enum stmt_type type() const;

		void set_instr(std::string);

		template<typename T> void add_param(T&&);

		std::string serialize() const;
	};

	class asm_instruction : public asm_statement {
	};

	class asm_type : public asm_statement {
	};

	class asm_label : public asm_statement {
	};

	class asm_size : public asm_statement {
	};

	class asm_section : public asm_statement {
	};

	class asm_comm : public asm_statement {
	};

	std::unique_ptr<asm_statement> parse_statement(std::string);

} // namespace assembly

#endif
