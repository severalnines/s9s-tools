#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
DIE=0

test -f $srcdir/configure.ac || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

mkdir -p m4 || true
autoreconf -vi

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
