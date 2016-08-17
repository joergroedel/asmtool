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

struct diff_options {
	bool show;
	bool pretty;
	int context;

	diff_options();
};

void diff_files(const char*, const char*, struct diff_options&);

#endif
