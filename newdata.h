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

	enum class symbol_type {
		FUNCTION,
		OBJECT,
		UNKNOWN,
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
		using token_t = std::vector<asm_token>;
		token_t m_tokens;

	public:
		template<typename T>
		void add_token(T&&);
		void reset();
		size_t tokens() const;
		void token(token_t::size_type,
			   std::function<void(enum token_type, std::string)>) const;
		std::string serialize() const;
	};

	class asm_statement {
	protected:
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

	class asm_type : public asm_statement {
	protected:
		enum symbol_type	m_stype;
		std::string		m_symbol;

	public:
		asm_type(std::string);
		enum symbol_type get_type() const;
		std::string get_symbol() const;
		virtual void analyze();
	};

	class asm_label : public asm_statement {
	public:
		asm_label(std::string);

		virtual void rename_label(std::string, std::string);

		std::string get_label() const;
	};

	class asm_size : public asm_statement {
	protected:
		std::string m_symbol;

	public:
		asm_size(std::string);

		virtual void analyze();

		std::string get_symbol() const;
	};

	class asm_section : public asm_statement {
	protected:
		std::string m_name;
		std::string m_flags;
		bool m_executable;

	public:
		asm_section(std::string);

		virtual void analyze();

		std::string get_name() const;
		bool executable() const;
	};

	class asm_comm : public asm_statement {
	protected:
		std::string	m_symbol;
		uint32_t	m_alignment;
		uint64_t	m_size;

	public:
		asm_comm(std::string);

		virtual void analyze();

		std::string get_symbol() const;
	};

	std::unique_ptr<asm_statement> parse_statement(std::string);

} // namespace assembly

#endif
