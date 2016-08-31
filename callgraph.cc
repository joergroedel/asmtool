/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>

#include "callgraph.h"
#include "assembly.h"
#include "helper.h"

using result_type = std::map<std::string, std::set<std::string> >;

static void cg_from_one_function(assembly::asm_file &file,
				 std::ofstream &of,
				 std::string fn_name)
{
	auto fn = file.get_function(fn_name, assembly::func_flags::STRIP_DEBUG);
	std::set<std::string> targets;

	if (fn == nullptr)
		return;

	fn->for_each_statement([&targets](assembly::asm_statement &stmt) {
		if (stmt.type() != assembly::stmt_type::INSTRUCTION)
			return;

		auto instr = stmt.instr();
		if (instr.size() < 4 || instr.substr(0, 4) != "call")
			return;

		// Now we have a call instruction - find the target
		stmt.param(0, [&targets](const assembly::asm_param &param) {
			if (!param.tokens()) {
				std::cerr << "Error: Empty param in call instruction" << std::endl;
				return;
			}
			param.token(0, [&targets](enum assembly::token_type type, std::string token) {
				if (type != assembly::token_type::IDENTIFIER)
					return;

				targets.insert(token);
			});
		});
	});

	int num = 0;

	of << '\t' << base_fn_name(fn_name) << " -> {";

	for (auto &target : targets) {
		if (num++)
			of << ", ";
		of << base_fn_name(target);
	}
	of << " };" << std::endl;
}

void generate_callgraph(const char *filename)
{
	const char *output_file = "callgraph.dot";
	std::ofstream of;

	assembly::asm_file file(filename);

	file.load();

	of.open(output_file);

	of << "digraph {" << std::endl;

	// Seems to produce better results
	of << "\trankdir=LR;" << std::endl;

	file.for_each_symbol([&of, &file](std::string sym, assembly::asm_symbol info) {

		// First check if this symbol is a function
		if (info.m_type != assembly::symbol_type::FUNCTION)
			return;

		cg_from_one_function(file, of, sym);
	});

	of << "}" << std::endl;
}
