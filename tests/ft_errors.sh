#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""

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
function testHelp01
{
    local exit_code

    $S9S 2>>$STDOUT_FILE >>$STDOUT_FILE
    exit_code=$?

    if [ "$VERBOSE" ]; then
        cat $STDOUT_FILE
        echo "*** exit_code: $exit_code"
    fi

    if [ $exit_code -ne 6 ]; then
        failure "The exit code is $exit_code while no command line options"
    fi

    rm -f $STDOUT_FILE
}

#
#
#
function testHelp02
{
    local exit_code

    $S9S --help >>$STDOUT_FILE
    exit_code=$?

    if [ "$VERBOSE" ]; then
        cat $STDOUT_FILE
        echo "*** exit_code: $exit_code"
    fi

    if [ $exit_code -ne 0 ]; then
        failure "The exit code is $exit_code while receiving the --help option."
    fi

    rm -f $STDOUT_FILE
}

#
# Running the requested tests.
#
startTests

if [ "$1" ]; then
    runFunctionalTest "$1"
else
    runFunctionalTest testHelp01
    runFunctionalTest testHelp02
fi

endTests


