#ifndef __DIFF_H
#define __DIFF_H

#include "asmfile.h"

void diff(asm_file *file1, asm_file *file2, std::ostream &os);
bool compare_functions(asm_file *file1, asm_function *f1,
		       asm_file *file2, asm_function *f2);

#endif
