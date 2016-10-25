#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
MYDIR=$(readlink -m "$MYDIR")
VERSION="0.0.4"
VERBOSE=""
LOGFILE=""
CONTAINER=""
SOURCE_SERVER=""
TARGET_SERVER=""

#
# Prints the software version and exits.
#
function printVersionAndExit()
{
    echo "$MYNAME Version $VERSION on $(hostname)" >&2
}


