asmtool - A Tool to Show Differences in GCC Generated Assembly Files
====================================================================

	$ make install

This will install the tool into $HOME/bin. After that you can use the 'diff'
sub-option to find functions that changed between the two versions of the
assembly file. An invocation could look like this:

	$ asmtool diff file_old.s file_new.s

This gives you a list of added, removed, and changed functions. To see what
changed the functions, use the -s option:

	$ asmtool diff -s file_old.s file_new.s

There are more options available to control the diff output. Use

	$ asmtool diff --help

to get an overview.
