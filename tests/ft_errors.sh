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
# This test script will run the s9s program without a single command line option
# and checks if it gives the proper error response.
#
function test01()
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

    checkMessage "$STDOUT_FILE" "Missing command line options"

    rm -f $STDOUT_FILE
}

#
# This test will send one single --help command line option and check what the
# result is. No error should be reported of course.
#
function test02()
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
# This test will try to pass an invalid mode and check the response.
#
function test03()
{
    local exit_code

    $S9S something 2>>$STDOUT_FILE >>$STDOUT_FILE
    exit_code=$?

    if [ "$VERBOSE" ]; then
        cat $STDOUT_FILE
        echo "*** exit_code: $exit_code"
    fi

    if [ $exit_code -ne 6 ]; then
        failure "The exit code is $exit_code while no command line options"
    fi

    checkMessage "$STDOUT_FILE" "is not a valid mode"

    rm -f $STDOUT_FILE
}

#
# This test checks what the exit code will be when using an invalid command line
# option.
#
function test04()
{
    local exit_code

    $S9S --cluster_name=ak 2>>$STDOUT_FILE >>$STDOUT_FILE
    exit_code=$?

    if [ "$VERBOSE" ]; then
        cat $STDOUT_FILE
        echo "*** exit_code: $exit_code"
    fi
    
    # Redirect is not working.
    if [ $exit_code -ne 6 ]; then
        failure "The exit code is $exit_code while using invalid option."
    fi
    
    checkMessage "$STDOUT_FILE" "unrecognized option"

    rm -f $STDOUT_FILE
}

#
# Running the requested tests.
#
startTests

if [ "$1" ]; then
    runFunctionalTest "$1"
else
    runFunctionalTest test01
    runFunctionalTest test02
    runFunctionalTest test03
    runFunctionalTest test04
fi

endTests


