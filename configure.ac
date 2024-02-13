AC_INIT([42sh], [0.1], [antonin.suzor@epita.fr])
AM_INIT_AUTOMAKE([subdir-objects] [foreign])

#CFLAGS= --coverage -O0
AC_PROG_CC

AM_SILENT_RULES
AM_PROG_AR
AC_PROG_RANLIB

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/ast/Makefile
	src/lexer/Makefile
	src/parser/Makefile
	src/variables/Makefile
	src/functions/Makefile
	src/IO_Backend/Makefile
	tests/Makefile
    ])

AC_OUTPUT
