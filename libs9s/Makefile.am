#
# Copyright (C) 2016-2018 severalnines.com
#
AM_LDFLAGS   = ""

AM_CPPFLAGS  = -Wall  -fno-strict-aliasing
AM_CPPFLAGS += -std=c++0x
AM_CPPFLAGS += -Wno-deprecated-declarations
AM_CPPFLAGS += -fPIC -pipe
#MY_LDFLAGS = -Wl,--whole-archive libs9s.a -Wl,--no-whole-archive

if COND_GCOV
   AM_CPPFLAGS += -fprofile-arcs -ftest-coverage --no-inline
endif

if COND_GCOV
    AM_LDFLAGS += -lgcov -coverage
endif

lib_LIBRARIES = libs9s.a

#
# Create a shared library (for faster linking)
#
libs9s.so: libs9s.a $(libs9s_a_OBJECTS)
if !DARWIN
	@echo "  CXXLD    $@"
	@$(CXX) $(AM_CPPFLAGS) -shared -pthread \
			-Wl,--whole-archive libs9s.a -Wl,--no-whole-archive -o $@

else
	@echo "  CXXLD    $@"
	@$(CXX) $(AM_CPPFLAGS) -shared -pthread \
			-Wl,-all_load libs9s.a  -L/usr/local/opt/openssl/lib/ -lcrypto -lssl -o $@
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

libs9s_public_h_headers =     \
	config_lexer.h            \
	config_parser.h           \
	json_lexer.h              \
	json_parser.h             \
	library.h                 \
	s9smutex.h                \
	s9smutexlocker.h          \
	s9sthread.h               \
	s9sobject.h               \
	s9ssqlprocess.h           \
	s9sprocess.h              \
	s9ssshcredentials.h       \
	s9saccount.h              \
	s9sbackup.h               \
	s9sbusinesslogic.h        \
	s9scluster.h              \
	s9scmongraph.h            \
	s9sconfigfile.h           \
	s9sconfigfile_p.h         \
	s9scontainer.h            \
	s9sevent.h                \
	s9sdatetime.h             \
	s9sdebug.h                \
	s9sdir.h                  \
	s9sfile.h                 \
	s9sfile_p.h               \
	s9sformat.h               \
	s9sformatter.h            \
	s9sglobal.h               \
	s9sgraph.h                \
	s9sgroup.h                \
	s9sjsonparsecontext.h     \
	s9smap.h                  \
	s9smessage.h              \
	s9salarm.h                \
	s9snode.h                 \
	s9sreplication.h          \
	s9sspreadsheet.h          \
	s9sserver.h               \
	s9sserver.cpp             \
	s9spkginfo.h              \
	s9spkginfo.cpp            \
	s9scontroller.h           \
	s9scontroller.cpp         \
	s9sjob.h                  \
	s9sjob.cpp                \
	s9soptions.h              \
	s9sparsecontext.h         \
	s9sparsecontextstate.h    \
	s9sregexp.h               \
	s9sregexp_p.h             \
	s9sreport.h               \
	s9srpcclient.h            \
	s9srpcclient_p.h          \
	s9srpcreply.h             \
	s9sdbgrowthreport.h       \
	s9srsakey.h               \
	s9srsakey_p.h             \
	s9sstack.h                \
	s9sstring.h               \
	s9sstringlist.h           \
	s9sdisplay.h              \
	s9swidget.h               \
	s9sbutton.h               \
	s9sdisplayentry.h         \
	s9sdisplaylist.h          \
	s9sdialog.h               \
	s9sentrydialog.h          \
	s9squestiondialog.h       \
	s9sbrowser.h              \
	s9sinfopanel.h            \
	s9seditor.h               \
	s9smonitor.h              \
	s9scommander.h            \
	s9scalc.h                 \
	s9stopui.h                \
	s9sunion.h                \
	s9surl.h                  \
	s9streenode.h             \
	s9suser.h                 \
	s9svariantarray.h         \
	s9svariant.h              \
	s9svariantlist.h          \
	s9svariantmap.h           \
	s9svector.h


libs9s_a_SOURCES =            \
	$(BUILT_SOURCES)          \
    $(libs9s_public_h_headers) \
	s9smutex.cpp               \
	s9smutexlocker.cpp         \
	s9sthread.cpp              \
	library.cpp               \
	s9sdebug.cpp              \
	s9sobject.cpp             \
	s9ssqlprocess.cpp         \
	s9sprocess.cpp            \
	s9ssshcredentials.cpp     \
	s9sstring.cpp             \
	s9sformat.cpp             \
	s9sformatter.cpp          \
	s9sdatetime.cpp           \
	s9surl.cpp                \
	s9salarm.cpp              \
	s9snode.cpp               \
	s9sreplication.cpp        \
	s9sspreadsheet.cpp        \
	s9scontainer.cpp          \
	s9sevent.cpp              \
	s9scluster.cpp            \
	s9sbackup.cpp             \
	s9streenode.cpp           \
	s9suser.cpp               \
	s9sreport.cpp             \
	s9sgroup.cpp              \
	s9smessage.cpp            \
	s9saccount.cpp            \
	s9svariant.cpp            \
	s9svariantmap.cpp         \
	s9svariantlist.cpp        \
	s9svariantarray.cpp       \
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
	s9sdbgrowthreport.cpp     \
	s9srpcclient_p.cpp        \
	s9srpcclient.cpp          \
	s9sbusinesslogic.cpp      \
	s9sdisplay.cpp            \
	s9swidget.cpp             \
	s9sbutton.cpp             \
	s9sdisplayentry.cpp       \
	s9sdisplaylist.cpp        \
	s9sdialog.cpp             \
	s9sentrydialog.cpp        \
	s9squestiondialog.cpp     \
	s9sbrowser.cpp            \
	s9sinfopanel.cpp          \
	s9seditor.cpp             \
	s9smonitor.cpp            \
	s9scommander.cpp          \
	s9scalc.cpp               \
	s9stopui.cpp              \
	s9sgraph.cpp              \
	s9scmongraph.cpp          \
	s9srsakey.cpp			  \
	s9srsakey_p.cpp			  \
	s9sconfigfile.cpp         \
	s9sconfigfile_p.cpp

libs9sdir = $(includedir)/s9s
libs9s_HEADERS = $(libs9s_public_h_headers)
