/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * Licensed under the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 *
 * See http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * for details.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <functional>
#include <iostream>
#include <string>

#include "assembly.h"

void show_symbol(const char *filename, const std::string &symbol)
{
	std::unique_ptr<assembly::asm_object> obj(nullptr);
	assembly::asm_file file(filename);

	file.load();

	if (file.has_function(symbol)) {
		obj = file.get_function(symbol, assembly::func_flags::STRIP_DEBUG);
	} else if (file.has_object(symbol)) {
		obj = file.get_object(symbol, assembly::func_flags::STRIP_DEBUG);
	} else {
		std::cerr << "Error: Symbol not found: " << symbol << std::endl;
		return;
	}

	std::cout << symbol << ":" << std::endl;

	obj->for_each_statement([](assembly::asm_statement &stmt) {
		std::string indent = "\t";

		if (stmt.type() == assembly::stmt_type::LABEL)
			indent = "";

		std::cout << indent << stmt.raw() << std::endl;
	});
}
