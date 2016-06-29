/*
 * Copyright (c) 2015 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __HELPER_H
#define __HELPER_H

#include <string>
#include <vector>

std::string trim(const std::string &line);
std::string strip_comment(const std::string &line);
std::vector<std::string> split_trim(const char *delim,
					   std::string line,
					   unsigned splits = 0);
bool generated_symbol(std::string symbol);
std::string expand_tab(std::string input);

#endif
