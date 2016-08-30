#include <iostream>
#include <iomanip>
#include <string>

#include "assembly.h"
#include "info.h"

void print_symbol_info(const char *filename, struct info_options opts)
{
	assembly::asm_file file(filename);

	file.load();

	file.for_each_symbol([&opts](std::string sym, assembly::asm_symbol info) {
		std::string scope;
		std::string type;

		if (info.m_scope == assembly::symbol_scope::LOCAL &&
		    !opts.local)
			return;

		switch (info.m_scope) {
		case assembly::symbol_scope::LOCAL:
			scope = "Local";
			break;
		case assembly::symbol_scope::GLOBAL:
			scope = "Global";
			break;
		default:
			scope = "Unknown";
			break;
		}

		switch (info.m_type) {
		case assembly::symbol_type::FUNCTION:
			type = "Function";
			break;
		case assembly::symbol_type::OBJECT:
			type = "Object";
			break;
		default:
			type = "Unknown";
			break;
		}

		std::cout << std::left;
		std::cout << "Symbol: " << std::setw(48) << sym;
		std::cout << " Scope: " << std::setw(10) << scope;
		std::cout << " Type: "  << std::setw(10) << type << std::endl;
	});
}
