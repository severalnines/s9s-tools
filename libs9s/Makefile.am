#
# Copyright (C) 2016 severalnines.com
#

AM_CPPFLAGS  = -Wall -Werror -fno-strict-aliasing
AM_CPPFLAGS += -Wno-deprecated-declarations
AM_CPPFLAGS += -fPIC -pipe

lib_LIBRARIES = libs9s.a

#
# Create a shared library (for faster linking)
#
libs9s.so: libs9s.a $(libs9s_a_OBJECTS)
	@echo "  CXXLD    $@"
	@$(CXX) $(AM_CPPFLAGS) -shared \
			-Wl,--whole-archive libs9s.a -Wl,--no-whole-archive -o $@

all-local: libs9s.so
	-@echo "Creating shared-objects."

SUFFIXES = parser.h parser.cpp lexer.h lexer.cpp
#
# Lexer
#
%lexer.cpp: %lexer.l
		@echo "  LEX	$@"
		@$(LEX) \
			-P$$(basename $@ lexer.cpp) \
			-o$@ $<

%lexer.h: %lexer.l
		@echo "  LEX	$@"
	    @$(LEX) \
			-P$$(basename $@ lexer.h) \
			-o/dev/null --header-file=$@ $<

#
# Parser
#
%parser.cpp: %parser.y
		@echo "  YACC	$@"
		@$(YACC) \
			-p $$(basename $@ parser.cpp) \
			--output=$@ $<

%parser.h: %parser.y
		@echo "  YACC	$@"
	    @$(YACC) \
			--defines=$@ \
			-p $$(basename $@ parser.h) $<

libs9s_a_SOURCES =            \
	library.cpp               \
	s9sdebug.cpp              \
	s9sstring.cpp             \
	s9svariant.cpp            \
	s9svariantmap.cpp         \
	s9svariantlist.cpp        \
	s9sstringlist.cpp         \
	s9sparsecontextstate.cpp 
