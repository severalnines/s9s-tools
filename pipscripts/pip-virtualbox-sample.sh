#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
MYDIR=$(readlink -m "$MYDIR")
VERSION="0.0.1"
VERBOSE=""
LOGFILE=""
SERVER=""

VBoxManage list runningvms --long

