#ifndef __NEWDATA_H

#include <vector>
#include <string>
#include <memory>
#include <map>

#include "generic-diff.h"

namespace assembly {

	enum class stmt_type {
		NOSTMT = 0,
		UNKNOWN,
		DOTFILE,
		INSTRUCTION,
		SECTION,
		TEXT,
		DATA,
		DATADEF,
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

	enum class symbol_scope {
		UNKNOWN,
		LOCAL,
		GLOBAL,
	};

	// Used as a bitfield
	enum class func_flags {
		NONE		= 0,
		NORMALIZE	= 1,
		STRIP_DEBUG	= 2,
	};

	using symbol_map = std::map<std::string, std::string>;

	constexpr enum func_flags operator|(const enum func_flags f1,
					    const enum func_flags f2)
	{
		return static_cast<enum func_flags>(
				static_cast<int>(f1) | static_cast<int>(f2));
	}

	constexpr enum func_flags operator&(const enum func_flags f1,
					    const enum func_flags f2)
	{
		return static_cast<enum func_flags>(
				static_cast<int>(f1) & static_cast<int>(f2));
	}

	class asm_token {
	protected:
		std::string	m_token;
		enum token_type	m_type;

	public:

		asm_token(std::string, enum token_type);
		enum token_type type() const;

		std::string token() const;
		void set(std::string);

		std::string serialize() const;
	};

	class asm_param {
	protected:
		using token_t			= std::vector<asm_token>;
		using token_handler		= std::function<void(asm_token&)>;
		using const_token_handler	= std::function<void(const asm_token&)>;
		token_t m_tokens;

	public:
		template<typename T>
		void add_token(T&&);
		void reset();
		size_t tokens() const;
		void token(token_t::size_type,
			   std::function<void(enum token_type, std::string)>) const;
		void for_each_token(token_handler);
		void for_each_token(const_token_handler) const;
		std::string serialize() const;
	};

	class asm_statement {
	protected:
		using param_type		= std::vector<asm_param>;
		using param_handler		= std::function<void(asm_param&)>;
		using const_param_handler	= std::function<void(const asm_param&)>;

		std::string		m_stmt;
		std::string		m_instr;
		enum stmt_type		m_type;
		param_type		m_params;

	public:
		asm_statement(std::string);
		virtual void rename_label(std::string, std::string);
		virtual void analyze();

		bool operator==(const asm_statement&) const;
		bool operator!=(const asm_statement&) const;

		void type(enum stmt_type);
		enum stmt_type type() const;

		std::string raw() const;

		void set_instr(std::string);

		template<typename T> void add_param(T&&);

		void param(param_type::size_type, param_handler);
		void param(param_type::size_type, const_param_handler) const;
		void for_each_param(param_handler);
		void for_each_param(const_param_handler) const;

		void map_symbols(symbol_map&, const asm_statement&) const;

		std::string serialize() const;
		std::string statement() const;
	};

	class asm_type : public asm_statement {
	protected:
		enum symbol_type	m_stype;
		std::string		m_symbol;

	public:
		asm_type(std::string);
		virtual void rename_label(std::string, std::string);
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
		virtual void rename_label(std::string, std::string);

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
		virtual void rename_label(std::string, std::string);

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
		virtual void rename_label(std::string, std::string);

		virtual void analyze();

		std::string get_symbol() const;
	};

	struct asm_symbol {
		size_t			m_idx;
		size_t			m_size_idx;
		size_t			m_section_idx;
		enum symbol_type	m_type;
		enum symbol_scope	m_scope;

		asm_symbol();
	};

	class asm_object : public diff::diffable<asm_statement> {
	protected:
		std::vector<std::unique_ptr<asm_statement>>	m_statements;
		std::string					m_name;

	public:
		asm_object(std::string);

		void add_statement(const std::unique_ptr<asm_statement> &stmt);

		void for_each_statement(std::function<void(asm_statement&)>);

		// Diffable interface
		virtual diff::size_type elements() const;
		virtual const asm_statement& element(diff::size_type) const;

		std::vector<std::string> get_symbols() const;
		void get_symbol_map(symbol_map&, const asm_object&) const;
	};

	class asm_file {
		std::vector<std::unique_ptr<asm_statement>>	m_statements;
		std::map<std::string, asm_symbol>		m_symbols;
		std::string					m_filename;

	public:
		template<typename T> inline asm_file(T&& fn)
			: m_filename(std::forward<T>(fn))
		{}

		void load();

		const asm_statement& stmt(unsigned) const;

		void for_each_symbol(std::function<void(std::string, asm_symbol)>);

		bool has_symbol(std::string) const;
		const asm_symbol& get_symbol(std::string) const;

		bool has_function(std::string) const;
		std::unique_ptr<asm_object> get_function(std::string, enum func_flags) const;

		bool has_object(std::string) const;
		std::unique_ptr<asm_object> get_object(std::string, enum func_flags) const;
	};

	using asm_diff = diff::diff<assembly::asm_statement>;

	std::unique_ptr<asm_statement> parse_statement(std::string);
	std::unique_ptr<asm_statement> copy_statement(const std::unique_ptr<asm_statement>&);

} // namespace assembly

#endif
