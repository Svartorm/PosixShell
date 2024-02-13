lib_LIBRARIES = liblexer.a

liblexer_a_SOURCES = lexer.c lexer.h token.c token.h expansion.c expansion.h
#liblexer_a_CFLAGS = -Wall -Wextra -Werror -Wvla -std=c99 -pedantic -g -fsanitize=address --coverage -O0
liblexer_a_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/ast \
	-I$(top_srcdir)/src/lexer \
	-I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/variables \
	-I$(top_srcdir)/src/IO_Backend
