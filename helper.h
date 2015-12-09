#ifndef __HELPER_H
#define __HELPER_H

#include <string>
#include <vector>

std::string trim(const std::string &line);
std::string strip_comment(const std::string &line);
std::vector<std::string> split_trim(const char *delim,
					   std::string line,
					   unsigned splits = 0);

#endif
