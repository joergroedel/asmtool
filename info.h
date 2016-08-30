#ifndef __INFO_H
#define __INFO_H

#include <string>

struct info_options {
	bool functions;
	bool objects;
	bool global;
	bool local;
	bool verbose;
	std::string fn_name;

	inline info_options()
		: functions(true), objects(false), global(true),
		  local(false), verbose(false), fn_name()
	{ }
};

void print_symbol_info(const char*, struct info_options);

#endif
