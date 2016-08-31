/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __CALLGRAPH_H
#define __CALLGRAPH_H

#include <string>

struct cg_options {
	std::string output_file;
	bool include_external;

	inline cg_options()
		: output_file("callgraph.dot"), include_external(false)
	{}
};

void generate_callgraph(const char *, const struct cg_options&);

#endif
