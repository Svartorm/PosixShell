bin_PROGRAMS = 42sh

42sh_SOURCES = main.c
#42sh_CFLAGS = -Wall -Wextra -Wvla -Werror -std=c99 -pedantic -g -fsanitize=address --coverage -O0
42sh_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/ast \
	-I$(top_srcdir)/src/lexer \
	-I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/variables \
	-I$(top_srcdir)/src/functions \
	-I$(top_srcdir)/src/IO_Backend

#42sh_LDFLAGS = -fsanitize=address
42sh_LDADD = \
	$(top_builddir)/src/ast/libast.a \
	$(top_builddir)/src/lexer/liblexer.a \
	$(top_builddir)/src/parser/libparser.a \
	$(top_builddir)/src/lexer/liblexer.a \
	$(top_builddir)/src/variables/libvariables.a \
	$(top_builddir)/src/functions/libfunctions.a \
	$(top_builddir)/src/IO_Backend/libio.a

SUBDIRS = ast lexer parser variables functions IO_Backend
