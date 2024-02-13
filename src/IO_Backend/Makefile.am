lib_LIBRARIES = libio.a

libio_a_SOURCES = io.c io.h
#libio_a_CFLAGS = -Wall -Wextra -Wvla -Werror -std=c99 -pedantic -g -fsanitize=address --coverage -O0
libio_a_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/ast \
	-I$(top_srcdir)/src/lexer \
	-I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/variables \
	-I$(top_srcdir)/src/IO_Backend
