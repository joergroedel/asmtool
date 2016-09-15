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

#ifndef __COPY_H
#define __COPY_H

#include <iostream>
#include <vector>
#include <string>

void copy_functions(const std::string&,
		    const std::vector<std::string>&,
		    std::ostream&);

#endif
