/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#include <iostream>
#include <string>
#include <vector>

#include "helper.h"

static size_t end_of_string(const std::string &line, size_t start)
{
	char c = line[start];
	size_t len = line.size();

	for (start += 1; start < len; start++) {
		if (line[start] == '\\')
			start += 1;
		else if (line[start] == c)
			break;
	}

	start += 1;

	if (start >= len)
		start = std::string::npos;

	return start;
}

std::string trim(const std::string &line)
{
	static const char *spaces = " \n\t\r";
	size_t pos1, pos2;

	pos1 = line.find_first_not_of(spaces);
	pos2 = line.find_last_not_of(spaces);

	if (pos1 == std::string::npos)
		return std::string("");

	return line.substr(pos1, pos2-pos1+1);
}

std::string strip_comment(const std::string &line)
{
	size_t pos = 0;

	while (true) {
		pos = line.find_first_of("#\"'", pos);
		if (pos == std::string::npos)
			return line;

		if (line[pos] == '#')
			return line.substr(0,pos);
		else
			pos = end_of_string(line, pos);
	}

	return line;
}

std::vector<std::string> split_trim(const char *delim, std::string line, unsigned splits)
{
	std::string item, delimiters;
	std::vector<std::string> items;
	unsigned num;

	line = trim(line);

	delimiters  = delim;
	delimiters += "\"'";
	num         = 0;

	for (size_t pos = 0; pos != std::string::npos;) {
		pos = line.find_first_of(delimiters.c_str(), pos);
		if (pos == std::string::npos) {
			item = line;
		} else if (line[pos] == '"' || line[pos] == '\'') {
			pos = end_of_string(line, pos);
			if (pos == std::string::npos)
				item = line;
			else
				continue;
		} else {
			item = line.substr(0, pos);
			line = line.substr(pos + 1);
			num += 1;
			pos  = 0;
		}

		items.push_back(trim(item));

		if (splits && num == splits) {
			items.push_back(trim(line));
			break;
		}
	}

	return items;
}

bool generated_symbol(std::string symbol)
{
	/*
	 * We consider symbols generated by the compiler
	 * when they contain a '.'
	 */
	return (symbol.find_first_of(".") != std::string::npos);
}

std::string expand_tab(std::string input)
{
	size_t index = 0;

	while (true) {
		index = input.find('\t', index);
		if (index == std::string::npos)
			break;

		size_t delta = (4 - (index % 4));
		const char *d_str;

		switch (delta) {
			case 0: d_str = ""; break;
			case 1: d_str = " "; break;
			case 2: d_str = "  "; break;
			case 3: d_str = "   "; break;
			case 4: d_str = "    "; break;
		}
		input.replace(index, 1, d_str);
	}

	return input;
}

std::string base_name(std::string fname)
{
	auto pos = fname.find_last_of("/");

	if (pos != std::string::npos)
		fname = fname.substr(pos + 1);

	return fname;
}

std::string base_fn_name(std::string fn_name)
{
	auto pos = fn_name.find_first_of(".");

	if (pos != std::string::npos)
		fn_name = fn_name.substr(0, pos);

	return fn_name;
}
