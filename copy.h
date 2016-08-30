/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * All rights reserved.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __COPY_H
#define __COPY_H

#include <iostream>
#include <vector>
#include <string>

void copy_functions(const std::string&,
		    const std::vector<std::string>&,
		    std::ostream&);

#endif
