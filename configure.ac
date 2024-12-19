# Process this file with autoconf to produce a configure script.
AC_INIT(s9s, 1.9.0)
AC_CONFIG_SRCDIR([libs9s/library.cpp])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT(/usr)

# Checks for programs.
AC_PROG_CPP
AC_PROG_CXX
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_MAKE_SET
AC_CHECK_PROGS([DOXYGEN], [doxygen])

AC_LANG([C++])

#
# We are building both the static and the shared object library.
#
#LT_INIT(shared static)

# Checks for libraries.
#AC_CHECK_LIB(m, trunc)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([strdup strtol strcmp strncmp])


# 
# Checking for libraries.
#
AC_CHECK_HEADERS([openssl/crypto.h],,AC_MSG_ERROR("Missing OpenSSL headers"))
AC_CHECK_LIB(crypto,EVP_EncryptUpdate,,AC_MSG_ERROR("libcrypto library not found."))
AC_CHECK_LIB(ssl,SSL_connect,,AC_MSG_ERROR("libssl library not found."))
LIBS+="-pthread"

#AC_CHECK_LIB(ncurses, initscr)

AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov],
    [use Gcov to test the test suite coverage])],
    [],
    [enable_gcov=no])
AM_CONDITIONAL([COND_GCOV],[test '!' "$enable_gcov" = no])

GIT_VERSION=$([LANG=C git rev-parse --short HEAD])
if test "x$GIT_VERSION" = "x"; then
    # no git client and .git directory for release builds (buildbot)
    GIT_VERSION="release"
fi
AC_DEFINE_UNQUOTED([GIT_VERSION], ["$GIT_VERSION"], [The git version hash])
AC_SUBST(GIT_VERSION)

BUILD_DATE=$([LC_ALL=C TZ=UTC date --rfc-3339=seconds])
AC_DEFINE_UNQUOTED([BUILD_DATE], ["$BUILD_DATE"], [The build date])
AC_SUBST(BUILD_DATE)

BUILD_YEAR=$([LC_ALL=C date +%Y])
AC_DEFINE_UNQUOTED([BUILD_YEAR], ["$BUILD_YEAR"], [The build year])
AC_SUBST(BUILD_YEAR)

BUILD_VERSION=$([LC_ALL=C TZ=UTC head -n1 debian/changelog | cut -d'(' -f2 | cut -d'-' -f1])
AC_DEFINE_UNQUOTED([BUILD_VERSION], ["$BUILD_VERSION"], [The build version])
AC_SUBST(BUILD_VERSION)

TESTS_DIR="tests"
AC_ARG_WITH(no-tests, AC_HELP_STRING([--with-no-tests], [You may disable the compilation of tests]),TESTS_DIR="")

AC_SUBST(TESTS_DIR)

RPATH_ARGS="-Wl,-rpath,. -Wl,-rpath,../libs9s -Wl,-rpath,./libs9s"
AC_ARG_WITH(no-rpath, AC_HELP_STRING([--with-no-rpath], [Disable the rpath defines]),RPATH_ARGS="")

AC_SUBST(RPATH_ARGS)

# Check for running on Darwin
AC_MSG_CHECKING([Checking if running on Darwin])
UNAME=`uname -s`
if test "x$UNAME" = "xDarwin"; then
  AC_DEFINE([IS_DARWIN], [1], [Set to 1 when running on Darwin - Mac OSX])
  AC_MSG_RESULT(yes)
  AC_SUBST([libsuffix],[".dylib"])
  is_darwin=true
else
  AC_MSG_RESULT(no)
  AC_SUBST([libsuffix],[".so"])
  is_darwin=false
fi
AM_CONDITIONAL([DARWIN], [test x$is_darwin = xtrue])

#
# Creating teh makefiles.
#
AC_CONFIG_FILES(Makefile                          \
                doc/Makefile                      \
                libs9s/Makefile                   \
                s9s/Makefile                      \
                tests/Makefile                    \
                tests/ut_library/Makefile         \
                tests/ut_s9sstring/Makefile       \
                tests/ut_s9sdatetime/Makefile     \
                tests/ut_s9surl/Makefile          \
                tests/ut_s9snode/Makefile         \
                tests/ut_s9sserver/Makefile       \
                tests/ut_s9scluster/Makefile      \
                tests/ut_s9sbackup/Makefile       \
                tests/ut_s9saccount/Makefile      \
                tests/ut_s9svariant/Makefile      \
                tests/ut_s9svariantmap/Makefile   \
                tests/ut_s9suser/Makefile         \
                tests/ut_s9sregexp/Makefile       \
                tests/ut_s9soptions/Makefile      \
                tests/ut_s9sgraph/Makefile        \
                tests/ut_s9srpcclient/Makefile    \
                tests/ut_s9sfile/Makefile         \
                tests/ut_s9sconfigfile/Makefile   \
               )

AC_OUTPUT
