# Process this file with autoconf to produce a configure script.
AC_INIT(s9s, 0.0.1)
AC_CONFIG_SRCDIR([libs9s/library.cpp])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AM_CONFIG_HEADER(config.h)
#AM_PROG_LIBTOOL
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
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([strdup strtol strcmp strncmp])

# for TLS/SSL
AC_CHECK_HEADERS([openssl/crypto.h openssl/tls1.h])
AC_CHECK_LIB(crypto,EVP_EncryptUpdate)
AC_CHECK_LIB(ssl,SSL_connect)

AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov],
    [use Gcov to test the test suite coverage])],
    [],
    [enable_gcov=no])
AM_CONDITIONAL([COND_GCOV],[test '!' "$enable_gcov" = no])

GIT_VERSION=$([LANG=C git log -1 --pretty=format:%H])
AC_DEFINE_UNQUOTED([GIT_VERSION], ["$GIT_VERSION"], [The git version hash])
AC_SUBST(GIT_VERSION)

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
                tests/ut_s9surl/Makefile          \
                tests/ut_s9snode/Makefile         \
                tests/ut_s9svariant/Makefile      \
                tests/ut_s9svariantmap/Makefile   \
                tests/ut_s9sregexp/Makefile       \
                tests/ut_s9soptions/Makefile      \
                tests/ut_s9srpcclient/Makefile    \
                tests/ut_s9sconfigfile/Makefile   \
               )

AC_OUTPUT
