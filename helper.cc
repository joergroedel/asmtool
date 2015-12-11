#include <iostream>
#include <string>
#include <vector>

#include "helper.h"

using namespace std;

static size_t end_of_string(const string &line, size_t start)
{
	char c = line[start];
	size_t len = line.size();

	for (start += 1; start < len; start++) {
		if (line[start] == '\\')
			start += 1;
		else if (line[start] == c)
			break;
	}

	if (start == len)
		start = string::npos;

	return start;
}

std::string trim(const std::string &line)
{
	static const char *spaces = " \n\t\r";
	size_t pos1, pos2;

	pos1 = line.find_first_not_of(spaces);
	pos2 = line.find_last_not_of(spaces);

	if (pos1 == std::string::npos)
		return string("");

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
			pos = end_of_string(line, pos) + 1;
	}

	return line;
}

vector<string> split_trim(const char *delim, string line, unsigned splits)
{
	string item, delimiters;
	vector<string> items;
	unsigned num;

	line = trim(line);

	delimiters  = delim;
	delimiters += "\"'";
	num         = 0;

	for (size_t pos = 0; pos != std::string::npos;) {
		pos = line.find_first_of(delimiters.c_str(), pos);
		if (pos == string::npos) {
			item = line;
		} else if (line[pos] == '"' || line[pos] == '\'') {
			pos = end_of_string(line, pos) + 1;
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

#if 0
	while (pos != std::string::npos) {
		pos = line.find_first_of(delim);
		if (pos != std::string::npos) {
			item = line.substr(0, pos);
			line = line.substr(pos+1);
		} else {
			item = line;
		}

		items.push_back(trim(item));
	}
#endif

	return items;
}
