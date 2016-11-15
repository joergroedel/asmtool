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

#ifndef __HELPER_H
#define __HELPER_H

#include <string>
#include <vector>

std::string trim(const std::string &line);
std::string strip_comment(std::string line);
std::vector<std::string> split_trim(const char *delim,
					   std::string line,
					   unsigned splits = 0);
bool generated_symbol(std::string symbol);
std::string expand_tab(std::string input);
std::string base_name(std::string fname);
std::string base_fn_name(std::string fn_name);

#endif
