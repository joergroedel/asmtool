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
