lib_LIBRARIES = libfunctions.a

libfunctions_a_SOURCES = functions.c functions.h ll_functions.c ll_functions.h hash_functions.c hash_functions.h
#libfunctions_a_CFLAGS = -Wall -Wextra -Wvla -Werror -std=c99 -pedantic -g -fsanitize=address --coverage -O0
libfunctions_a_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/ast \
	-I$(top_srcdir)/src/lexer \
	-I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/variables \
	-I$(top_srcdir)/src/functions \
	-I$(top_srcdir)/src/IO_Backend
