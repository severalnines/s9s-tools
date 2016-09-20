#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)

STDOUT_FILE=ft_errors.log

#if [ ! -d data -a -d tests/data ]; then
#    echo "Entering directory tests..."
#    cd tests
#fi

cd $MYDIR
source include.sh

function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]
 Test script for s9s to check various error conditions.

 -h, --help      Print this help and exit.
 --verbose       Print more messages.

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

if [ -z "$S9S" ]; then
    echo "The s9s program is not installed."
    exit 7
fi

#if [ ! -d data -a -d tests/data ]; then
#    echo "Entering directory tests..."
#    cd tests
#fi

#
#
#
function test01
{
    AK=1
}

#
# Running the requested tests.
#
startTests

if [ "$1" ]; then
    runFunctionalTest "$1"
else
    runFunctionalTest test01
fi

endTests


