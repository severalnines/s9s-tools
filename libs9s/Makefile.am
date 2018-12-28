#
# Copyright (C) 2016-2018 severalnines.com
#

AM_CPPFLAGS  = -Wall -Werror -fno-strict-aliasing
AM_CPPFLAGS += -std=c++0x
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
	S9sMutex                  \
	s9smutex.h                \
	S9sMutexLocker            \
	s9smutexlocker.h          \
	S9sThread                 \
	s9sthread.h               \
	S9sObject                 \
	s9sobject.h               \
	S9sProcess                \
	s9sprocess.h              \
	S9sSshCredentials         \
	s9ssshcredentials.h       \
	S9sAccount                \
	s9saccount.h              \
	S9sBackup                 \
	s9sbackup.h               \
	S9sBusinessLogic          \
	s9sbusinesslogic.h        \
	S9sCluster                \
	s9scluster.h              \
	S9sCmonGraph              \
	s9scmongraph.h            \
	S9sConfigFile             \
	s9sconfigfile.h           \
	s9sconfigfile_p.h         \
	S9sContainer              \
	s9scontainer.h            \
	S9sEvent                  \
	s9sevent.h                \
	S9sDateTime               \
	s9sdatetime.h             \
	s9sdebug.h                \
	S9sDir                    \
	s9sdir.h                  \
	S9sFile                   \
	s9sfile.h                 \
	s9sfile_p.h               \
	S9sFormat                 \
	s9sformat.h               \
	S9sFormatter              \
	s9sformatter.h            \
	S9sGlobal                 \
	s9sglobal.h               \
	S9sGraph                  \
	s9sgraph.h                \
	S9sGroup                  \
	s9sgroup.h                \
	S9sJsonParseContext       \
	s9sjsonparsecontext.h     \
	S9sMap                    \
	s9smap.h                  \
	S9sMessage                \
	s9smessage.h              \
	S9sAlarm                  \
	s9salarm.h                \
	S9sNode                   \
	s9snode.h                 \
	S9sSpreadsheet            \
	s9sspreadsheet.h          \
	S9sServer                 \
	s9sserver.h               \
	s9sserver.cpp             \
	S9sJob                    \
	s9sjob.h                  \
	s9sjob.cpp                \
	S9sOptions                \
	s9soptions.h              \
	S9sParseContext           \
	s9sparsecontext.h         \
	S9sParseContextState      \
	s9sparsecontextstate.h    \
	S9sRegExp                 \
	s9sregexp.h               \
	s9sregexp_p.h             \
	S9sReport                 \
	s9sreport.h               \
	S9sRpcClient              \
	s9srpcclient.h            \
	s9srpcclient_p.h          \
	S9sRpcReply               \
	s9srpcreply.h             \
	S9sRsaKey                 \
	s9srsakey.h               \
	s9srsakey_p.h             \
	S9sStack                  \
	s9sstack.h                \
	S9sString                 \
	s9sstring.h               \
	S9sStringList             \
	s9sstringlist.h           \
	S9sDisplay                \
	s9sdisplay.h              \
	S9sWidget                 \
	s9swidget.h               \
	S9sDisplayEntry           \
	s9sdisplayentry.h         \
	S9sDisplayList            \
	s9sdisplaylist.h          \
	S9sDialog                 \
	s9sdialog.h               \
	S9sEntryDialog            \
	s9sentrydialog.h          \
	S9sQuestionDialog         \
	s9squestiondialog.h       \
	S9sBrowser                \
	s9sbrowser.h              \
	S9sInfoPanel              \
	s9sinfopanel.h            \
	S9sMonitor                \
	s9smonitor.h              \
	S9sCommander              \
	s9scommander.h            \
	S9sCalc                   \
	s9scalc.h                 \
	S9sTopUi                  \
	s9stopui.h                \
	s9sunion.h                \
	S9sUrl                    \
	s9surl.h                  \
	S9sTreeNode               \
	s9streenode.h             \
	S9sUser                   \
	s9suser.h                 \
	S9sVariant                \
	S9sVariantArray           \
	s9svariantarray.h         \
	s9svariant.h              \
	S9sVariantList            \
	s9svariantlist.h          \
	S9sVariantMap             \
	s9svariantmap.h           \
	S9sVector                 \
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
	s9sprocess.cpp            \
	s9ssshcredentials.cpp     \
	s9sstring.cpp             \
	s9sformat.cpp             \
	s9sformatter.cpp          \
	s9sdatetime.cpp           \
	s9surl.cpp                \
	s9salarm.cpp              \
	s9snode.cpp               \
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
	s9srpcclient_p.cpp        \
	s9srpcclient.cpp          \
	s9sbusinesslogic.cpp      \
	s9sdisplay.cpp            \
	s9swidget.cpp             \
	s9sdisplayentry.cpp       \
	s9sdisplaylist.cpp        \
	s9sdialog.cpp             \
	s9sentrydialog.cpp        \
	s9squestiondialog.cpp     \
	s9sbrowser.cpp            \
	s9sinfopanel.cpp          \
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
