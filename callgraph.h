/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __CALLGRAPH_H
#define __CALLGRAPH_H

#include <vector>
#include <string>

struct cg_options {
	std::vector<const char*> input_files;
	std::vector<std::string> functions;
	std::string output_file;
	bool include_external;
	unsigned maxdepth;

	inline cg_options()
		: output_file("callgraph.dot"), include_external(false),
		  maxdepth(~0)
	{}
};

void generate_callgraph(const struct cg_options&);

#endif
