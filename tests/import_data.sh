#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
SQL_HOST=""

cd $MYDIR
source ./include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests the cluster under load.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose" \
        -- "$@")

if [ $? -ne 0 ]; then
    exit 6
fi

eval set -- "$ARGS"
while true; do
    case "$1" in
        -h|--help)
            shift
            printHelpAndExit
            ;;

        --verbose)
            shift
            VERBOSE="true"
            ;;

        --)
            shift
            break
            ;;
    esac
done

if [ -z "$SQL_HOST" ]; then
    SQL_HOST=$(\
        s9s node --list --properties="class_name=CmonGaleraHost" | \
        head -n1)
fi

if [ -z "$SQL_HOST" ]; then
    printError "Could not find SQL host."
    exit 5
fi

if [ -z "$SQL_PORT" ]; then
    SQL_PORT=$(s9s node --list --node-format="%P\n" "$SQL_HOST")
fi

if [ -z "$SQL_PORT" ]; then
    printError "Could not find SQL port."
    exit 5
fi

printVerbose " SQL_HOST : '$SQL_HOST'"
printVerbose " SQL_PORT : $SQL_PORT"
