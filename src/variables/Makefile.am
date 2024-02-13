lib_LIBRARIES = libvariables.a

libvariables_a_SOURCES = variables.c variables.h ll_variables.c ll_variables.h hash_variables.c hash_variables.h shell_variables.c shell_variables.h
#libvariables_a_CFLAGS = -Wall -Wextra -Wvla -Werror -std=c99 -pedantic -g -fsanitize=address --coverage -O0
libvariables_a_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/ast \
	-I$(top_srcdir)/src/lexer \
	-I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/variables \
	-I$(top_srcdir)/src/IO_Backend
