#include <iostream>
#include <iomanip>
#include <string>

#include "assembly.h"
#include "info.h"

static void print_symbols(assembly::asm_file &file, struct info_options &opts,
			  std::function<bool(assembly::asm_symbol&)> filter)
{
	file.for_each_symbol([&opts, &filter](std::string sym, assembly::asm_symbol info) {
		std::string scope;
		std::string type;

		if (!filter(info))
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
			type = "Function:";
			break;
		case assembly::symbol_type::OBJECT:
			type = "Object:";
			break;
		default:
			type = "Unknown:";
			break;
		}

		std::cout << std::left;
		std::cout << std::setw(10) << type << std::setw(48) << sym;
		std::cout << " Scope: " << std::setw(10) << scope << std::endl;
	});
}

void print_symbol_info(const char *filename, struct info_options opts)
{
	assembly::asm_file file(filename);

	file.load();

	if (opts.functions && opts.global)
	print_symbols(file, opts, [](assembly::asm_symbol &s)
		{ return s.m_type == assembly::symbol_type::FUNCTION &&
			 s.m_scope == assembly::symbol_scope::GLOBAL; });

	if (opts.functions && opts.local)
		print_symbols(file, opts, [](assembly::asm_symbol &s)
			{ return s.m_type == assembly::symbol_type::FUNCTION &&
				 s.m_scope == assembly::symbol_scope::LOCAL; });

	if (opts.objects && opts.global)
	print_symbols(file, opts, [](assembly::asm_symbol &s)
		{ return s.m_type == assembly::symbol_type::OBJECT &&
			 s.m_scope == assembly::symbol_scope::GLOBAL; });

	if (opts.objects && opts.local)
		print_symbols(file, opts, [](assembly::asm_symbol &s)
			{ return s.m_type == assembly::symbol_type::OBJECT &&
				 s.m_scope == assembly::symbol_scope::LOCAL; });
}
