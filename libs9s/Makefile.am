#
# Copyright (C) 2016 severalnines.com
#

AM_CPPFLAGS  = -Wall -Werror -fno-strict-aliasing
AM_CPPFLAGS += -Wno-deprecated-declarations
AM_CPPFLAGS += -fPIC -pipe
MY_LDFLAGS = -Wl,--whole-archive libs9s.a -Wl,--no-whole-archive
lib_LIBRARIES = libs9s.a

#
# Create a shared library (for faster linking)
#
libs9s.so: libs9s.a $(libs9s_a_OBJECTS)
if !DARWIN
	@echo "  CXXLD    $@"
	@$(CXX) $(AM_CPPFLAGS) -shared \
			-Wl,--whole-archive libs9s.a -Wl,--no-whole-archive -o $@

else
	@echo "  CXXLD    $@"
	@$(CXX) $(AM_CPPFLAGS) -shared \
			-Wl,-all_load libs9s.a  -L/usr/local/opt/openssl/lib/  -lcrypto -lssl -o $@
endif

all-local: libs9s.so
	-@echo "Creating shared-objects."


install-exec-hook: libs9s.so
	-@echo " $(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) libs9s.so '$(DESTDIR)$(libdir)'"
	$(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) libs9s.so "$(DESTDIR)$(libdir)"

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

BUILT_SOURCES =               \
    json_lexer.h              \
    json_lexer.cpp            \
    json_parser.h             \
    json_parser.cpp           \
    config_lexer.h            \
    config_lexer.cpp          \
    config_parser.h           \
    config_parser.cpp           

libs9s_a_SOURCES =            \
	$(BUILT_SOURCES)          \
	library.cpp               \
	s9sdebug.cpp              \
	s9sstring.cpp             \
	s9sformat.cpp             \
	s9sdatetime.cpp           \
	s9surl.cpp                \
	s9snode.cpp               \
	s9scluster.cpp            \
	s9svariant.cpp            \
	s9svariantmap.cpp         \
	s9svariantlist.cpp        \
	s9sstringlist.cpp         \
	s9sparsecontextstate.cpp  \
	s9sparsecontext.cpp       \
	s9sjsonparsecontext.cpp   \
	s9soptions.cpp            \
	s9sfile_p.cpp             \
	s9sfile.cpp               \
	s9sdir.cpp                \
	s9sregexp_p.cpp           \
	s9sregexp.cpp             \
	s9srpcreply.cpp           \
	s9srpcclient_p.cpp        \
	s9srpcclient.cpp          \
	s9sbusinesslogic.cpp      \
	s9stopui.cpp              \
	s9srsakey.cpp			  \
	s9srsakey_p.cpp			  \
	s9sconfigfile.cpp         \
	s9sconfigfile_p.cpp


