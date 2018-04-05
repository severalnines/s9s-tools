#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CONTAINER_SERVER=""
CONTAINER_IP=""

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]
 Test script for s9s to check various error conditions.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:" \
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
            OPTION_VERBOSE="--verbose"
            ;;

        --log)
            shift
            LOG_OPTION="--log"
            ;;

        --print-json)
            shift
            OPTION_PRINT_JSON="--print-json"
            ;;

        --print-commands)
            shift
            DONT_PRINT_TEST_MESSAGES="true"
            PRINT_COMMANDS="true"
            ;;

        --reset-config)
            shift
            OPTION_RESET_CONFIG="true"
            ;;

        --server)
            shift
            CONTAINER_SERVER="$1"
            shift
            ;;

        --)
            shift
            break
            ;;
    esac
done

if [ -z "$S9S" ]; then
    printError "The s9s program is not installed."
    exit 7
fi

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

#
# This test will create a nested folder structure in one step. Then it will
# check if the folders are there and the hidden entries are indeed not shown
# without the --all command line option.
#
function testMkdir1()
{
    local lines
    local expected

    print_title "Creating Folders"

    mys9s tree --mkdir /home/pipas/.config

    check_exit_code_no_job $?

    mys9s tree --tree
    mys9s tree --list

    lines=$(s9s tree --list)
    expected="home$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi

    lines=$(s9s tree --list --recursive --full-path --all)
    expected="/home/pipas$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi
    
    expected="./home/pipas/.config$"
    if echo "$lines" | grep --quiet "$expected"; then
        failure "Hidden entries should not be seen ('$expected')"
        exit 1
    fi
}

function testMkdir2()
{
    local lines
    local owner
    local group
    local name

    print_title "Creating Folders with Failures"

    mys9s tree --mkdir /testMkdir2
    check_exit_code_no_job $?
    
    lines=$(s9s tree --list --long | grep testMkdir2)
    owner=$(echo "$lines" | awk '{print $3}')
    group=$(echo "$lines" | awk '{print $4}')
    name=$(echo "$lines" | awk '{print $5}')
    
    if [ "$owner" != "pipas" ]; then
        failure "Owner is '$owner', should be 'pipas'"
        exit 1
    fi
    
    if [ "$group" != "testgroup" ]; then
        failure "Group is '$group', should be 'testgroup'"
        exit 1
    fi
    
    if [ "$name" != "testMkdir2" ]; then
        failure "Name is '$name', should be 'testMkdir2'"
        exit 1
    fi
    
    # We should not be able to create a folder that already exists.
    mys9s tree --mkdir /testMkdir2

    if [ $? -eq 0 ]; then
        failure "Creating the same folder should have failed"
        exit 1
    fi

    return 0
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testMkdir1
    runFunctionalTest testMkdir2
fi

endTests
