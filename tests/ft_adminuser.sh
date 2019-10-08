#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

export S9S_SYSTEM_CONFIG="/tmp/cmon_etc/s9s.conf"
export S9S_USER_CONFIG="$HOME/.s9s/$MYBASENAME.conf"

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests the availability of the admin user.

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

function testConfig()
{
    if [ -z "$S9S_SYSTEM_CONFIG" ]; then
        failure "The S9S_SYSTEM_CONFIG variable is not set."
        return 1
    fi

    if [ ! -f "$S9S_SYSTEM_CONFIG" ]; then
        failure "The file '$S9S_SYSTEM_CONFIG' is not found."
    fi

    echo "Using config file '$S9S_SYSTEM_CONFIG'."
}

#
# Checking that the current user (created in grant_user()) can log in and can
# view its own public key.
#
function testUser()
{
    local userName="admin"
    local myself

    #
    # 
    #
    print_title "Testing --whoami"

    mys9s user --whoami
    myself=$(s9s user --whoami)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with public key ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi

    mys9s user --list --long
}

#
# Running the requested tests.
#
startTests
#reset_config --do-not-create
create_s9s_config

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testConfig
    runFunctionalTest testUser
fi

endTests


