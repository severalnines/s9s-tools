#
# Copyright (C) 2016-2018 severalnines.com
#

AM_CPPFLAGS = -Wall -Werror -fno-strict-aliasing
AM_CPPFLAGS += -Wno-deprecated-declarations
AM_CPPFLAGS += -I../
AM_CPPFLAGS += -I../libs9s/
AM_CPPFLAGS += -I../libs9s/include/
AM_CPPFLAGS += -pipe

LDADD = -L../libs9s -ls9s -lrt -lpthread

#add '../libs9s' to the lib search path (for normal builds)
s9s_LDFLAGS = -pthread $(RPATH_ARGS)

if COND_GCOV
   AM_CPPFLAGS += -fprofile-arcs -ftest-coverage --no-inline
endif

if COND_GCOV
    s9s_LDFLAGS += -lgcov -coverage
endif

bin_PROGRAMS = s9s

s9s_SOURCES =            \
	main.cpp


