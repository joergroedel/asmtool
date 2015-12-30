/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __DIFF_H
#define __DIFF_H

#include <string>
#include <map>

#include "asmfile.h"

void diff(asm_file *file1, asm_file *file2, std::ostream &os);
bool compare_functions(asm_file *file1, asm_function *f1,
		       asm_file *file2, asm_function *f2,
		       std::map<std::string, std::string> &symbol_map);

#endif
