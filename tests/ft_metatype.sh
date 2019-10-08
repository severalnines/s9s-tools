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

FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh

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

#
# This function is used to cut one cell (of a given row and column) from a table
# formatted text.
#
function index_table()
{
    local text="$1"
    local row=$2
    local column=$3
    local line
    local counter=1

    line=$(echo "$text" | head -n $row | tail -n 1)
    for cell in $line; do
        if [ $counter -eq "$column" ]; then
            echo "$cell"
            break
        fi

        let counter+=1
    done
}

#
# This test will check the system users, users that should be available on every
# system.
#
function testTypes()
{
    local typeName
    local output
    local cmonAccountFound
    local cmonVerbatimFound

    print_title "Testing types"
    mys9s metatype --list

    output=$(s9s metatype --list)
    for typeName in $output; do
        if [ "$typeName" == "CmonAccount" ]; then
            cmonAccountFound="true"
        fi
        
        if [ "$typeName" == "CmonVerbatim" ]; then
            cmonVerbatimFound="true"
        fi
    done

    if [ -z "$cmonAccountFound" ]; then
        failure "The type 'CmonAccount' was not found."
    fi
    
    if [ -z "$cmonVerbatimFound" ]; then
        failure "The type 'CmonVerbatim' was not found."
    fi

    output=$(s9s metatype --list --long --no-header CmonStats)
    if ! echo "$output" | grep -q "Basic type for"; then
        failure "Metatype Long list does not match."
    fi

    return 0
}

#
# Testing the metatype properties.
#
function testProperties()
{
    local output
    local userFound
    local hostNameFound
    local propertyName

    print_title "Testing properties"
    mys9s metatype --list-properties --type=CmonFile

    output=$(s9s metatype --list-properties --type=CmonFile)
    for propertyName in $output; do
        #echo "-> $propertyName"
        if [ "$propertyName" == "user" ]; then
            userFound="true"
        fi

        if [ "$propertyName" == "host_name" ]; then
            hostNameFound="true"
        fi
    done

    if [ -z "$userFound" ]; then
        failure "The property 'user' was not found."
    fi
    
    if [ -z "$hostNameFound" ]; then
        failure "The property 'host_name' was not found."
    fi

    mys9s metatype --list-properties --type=CmonFile --long user

    output=$(s9s metatype --list-properties --type=CmonFile --long --batch user)
    if ! echo "$output" | grep -q "The name of the owner"; then
        failure "Property long list does not match."
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
    #runFunctionalTest testPing
    runFunctionalTest testTypes
    runFunctionalTest testProperties

fi

endTests


