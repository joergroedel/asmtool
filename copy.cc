#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "assembly.h"
#include "copy.h"

static void copy_symbol(const std::string &symbol,
			const assembly::asm_file &file,
			std::ostream &os)
{
	if (!file.has_symbol(symbol)) {
		std::cerr << "Error: Symbol not found: " << symbol << std::endl;
		return;
	}

	auto func = std::unique_ptr<assembly::asm_object>(nullptr);
	auto sym  = file.get_symbol(symbol);

	if (file.has_function(symbol)) {
		func = file.get_function(symbol, assembly::func_flags::STRIP_DEBUG);
	} else if (file.has_object(symbol)) {
		func = file.get_object(symbol, assembly::func_flags::STRIP_DEBUG);
	} else {
		std::cerr << "Symbol not found: " << symbol << std::endl;
		return;
	}

	if (sym.m_section_idx)
		os << '\t' << file.stmt(sym.m_section_idx).raw() << std::endl;

	if (sym.m_align_idx)
		os << '\t' << file.stmt(sym.m_align_idx).raw() << std::endl;

	if (sym.m_type_idx)
		os << '\t' << file.stmt(sym.m_type_idx).raw() << std::endl;

	if (sym.m_type == assembly::symbol_type::OBJECT && sym.m_size_idx)
		os << '\t' << file.stmt(sym.m_size_idx).raw() << std::endl;

	os << symbol << ':' << std::endl;
	func->for_each_statement([&os](assembly::asm_statement &stmt) {
		std::string prefix(stmt.type() == assembly::stmt_type::LABEL ? "" : "\t");
		os << prefix << stmt.raw() << std::endl;
	});

	if (sym.m_type == assembly::symbol_type::FUNCTION && sym.m_size_idx)
		os << '\t' << file.stmt(sym.m_size_idx).raw() << std::endl;

	// TODO:
	//   * Alignment handling
	//   * .type handling

}

void copy_functions(const std::string &filename,
		    const std::vector<std::string> &symbols,
		    std::ostream &os)
{
	std::set<std::string> functions, objects;
	assembly::asm_file file(filename);


	file.load();

	for (auto fn : symbols) {

		if (!file.has_function(fn)) {
			std::cerr << "Function not found: " << fn << std::endl;
			continue;
		}

		auto func = file.get_function(fn, assembly::func_flags::STRIP_DEBUG);
		std::vector<std::string> syms = func->get_symbols();

		for (auto s : syms) {
			if (!file.has_symbol(s))
				continue;

			auto sym = file.get_symbol(s);

			if (sym.m_scope != assembly::symbol_scope::LOCAL)
				continue;

			if (sym.m_type == assembly::symbol_type::FUNCTION)
				functions.emplace(s);
			else if (sym.m_type == assembly::symbol_type::OBJECT)
				objects.emplace(s);
		}

		copy_symbol(fn, file, os);
	}


	for (auto fn : functions) {
		copy_symbol(fn, file, os);
		os << "\t.globl " << fn << std::endl;
	}

	for (auto obj : objects)
		copy_symbol(obj, file, os);
}
