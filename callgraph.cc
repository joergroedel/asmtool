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
				 std::string fn_name,
				 result_type &result,
				 const struct cg_options &opts)
{
	auto fn = file.get_function(fn_name, assembly::func_flags::STRIP_DEBUG);
	std::string rs_name = base_fn_name(fn_name);

	if (fn == nullptr)
		return;

	fn->for_each_statement([&result, &rs_name, &opts](assembly::asm_statement &stmt) {
		if (stmt.type() != assembly::stmt_type::INSTRUCTION)
			return;

		auto instr = stmt.instr();
		if (instr.size() < 4 || instr.substr(0, 4) != "call")
			return;

		// Now we have a call instruction - find the target
		stmt.param(0, [&result, &rs_name, &opts](const assembly::asm_param &param) {
			if (!param.tokens()) {
				std::cerr << "Error: Empty param in call instruction" << std::endl;
				return;
			}
			param.token(0, [&result, &rs_name, &opts](enum assembly::token_type type, std::string token) {
				if (type != assembly::token_type::IDENTIFIER)
					return;

				std::string insert_token = base_fn_name(token);

				if ((result.find(insert_token) == result.end()) &&
				    !opts.include_external)
					return;

				result[rs_name].insert(insert_token);
			});
		});
	});
}

void generate_callgraph(const struct cg_options &opts)
{
	const char *output_file = opts.output_file.c_str();
	std::map<std::string, size_t> sym_file_map;
	std::vector<assembly::asm_file> files;
	result_type results;
	std::ofstream of;

	for (auto fn : opts.input_files)
		files.emplace_back(fn);

	for (auto &file : files)
		file.load();

	of.open(output_file);

	for (size_t idx = 0, size = files.size(); idx != size; ++idx) {
		// First fill the results with known symbols
		files[idx].for_each_symbol([&results, &opts, &idx, &sym_file_map]
				     (std::string sym, assembly::asm_symbol info) {
			// First check if this symbol is a function
			if (info.m_type != assembly::symbol_type::FUNCTION)
				return;

			std::string base_name = base_fn_name(sym);

			results[base_name] = std::set<std::string>();
			sym_file_map[base_name] = idx;
		});
	}

	for (size_t idx = 0, size = files.size(); idx != size; ++idx) {
		files[idx].for_each_symbol([&files, &results, &opts, &idx]
					   (std::string sym, assembly::asm_symbol info) {
			cg_from_one_function(files[idx], sym, results, opts);
		});
	}

	of << "digraph {" << std::endl;
	// rankdir=Lr seems to produce better results
	of << "\trankdir=LR;" << std::endl;

	for (auto &r : results) {
		int num = 0;

		of << '\t' << r.first << " -> {";
		for (auto &s : r.second) {
			if (num++)
				of << ", ";
			of << s;
		}
		of << '}' << std::endl;
	}

	of << "}" << std::endl;
}
