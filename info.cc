#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

#include "assembly.h"
#include "info.h"

static void print_one_symbol(assembly::asm_file &file,
			     std::string &sym, assembly::asm_symbol &info,
			     bool verbose)
{
	std::string scope;
	std::string type;

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

	if (!verbose)
		return;

	std::unique_ptr<assembly::asm_object> obj(nullptr);

	if (info.m_type == assembly::symbol_type::FUNCTION)
		obj = file.get_function(sym, assembly::func_flags::STRIP_DEBUG);
	else
		obj = file.get_object(sym, assembly::func_flags::STRIP_DEBUG);

	if (obj == nullptr)
		return;

	auto symbols = obj->get_symbols();

	if (symbols.size() == 0)
		return;

	std::cout << "    Referenced symbols " << std::endl;
	std::cout << "    (F - Function, O - Object, G - Global, L - Local, E - External):" << std::endl;

	for (auto &s : symbols) {
		char type = '-', scope = 'E';

		if (file.has_symbol(s)) {
			auto sym = file.get_symbol(s);

			if (sym.m_type == assembly::symbol_type::FUNCTION)
				type = 'F';
			else if (sym.m_type == assembly::symbol_type::OBJECT)
				type = 'O';

			if (sym.m_scope == assembly::symbol_scope::GLOBAL)
				scope = 'G';
			else if (sym.m_scope == assembly::symbol_scope::LOCAL)
				scope = 'L';
		}

		std::cout << "        " << s << " (" << type << scope << ')' << std::endl;
	}
}

static void print_symbols(assembly::asm_file &file, struct info_options &opts,
			  std::function<bool(assembly::asm_symbol&)> filter)
{
	file.for_each_symbol([&file, &opts, &filter](std::string sym,
						     assembly::asm_symbol info) {
		if (!filter(info))
			return;

		print_one_symbol(file, sym, info, opts.verbose);
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
