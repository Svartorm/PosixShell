lib_LIBRARIES = libast.a

libast_a_SOURCES = ast.c ast.h ast_exec.c ast_exec.h
#libast_a_CFLAGS = -Wall -Wextra -Wvla -Werror -std=c99 -pedantic -g -fsanitize=address --coverage -O0
libast_a_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/ast \
	-I$(top_srcdir)/src/lexer \
	-I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/variables \
	-I$(top_srcdir)/src/IO_Backend
