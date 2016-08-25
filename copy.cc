#include <iostream>
#include <string>
#include <vector>

#include "assembly.h"
#include "copy.h"

void copy_functions(const std::string &filename,
		    const std::vector<std::string> &symbols,
		    std::ostream &os)
{
	assembly::asm_file file(filename);

	file.load();

	for (auto fn : symbols) {
		if (!file.has_function(fn))
			std::cerr << "Function not found: " << fn << std::endl;

		auto func = file.get_function(fn, assembly::func_flags::STRIP_DEBUG);

		os << fn << ':' << std::endl;
		func->for_each_statement([&os](assembly::asm_statement &stmt) {
			std::string prefix(stmt.type() == assembly::stmt_type::LABEL ? "" : "\t");
			os << prefix << stmt.raw() << std::endl;
		});

		// TODO:
		//   * Section handling
		//   * .size statements
		//   * Alignment handling
		//   * Copy referenced local symbols
	}
}
