#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")

# This is the name of the server that will hold the linux containers.
CONTAINER_SERVER="core1"

FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

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
            SERVER="$1"
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

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

reset_config

#
# 
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
#
#
function testPing()
{
    pip-say "Pinging controller."

    #
    # Pinging. 
    #
    $S9S cluster \
        --ping \
        $OPTION_PRINT_JSON \
        $OPTION_VERBOSE

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
        pip-say "The controller is off line. Further testing is not possible."
    else
        pip-say "The controller is on line."
    fi
}

#
# Checks the main operation command line options for the job handling.
#
function testJobOperations()
{
    local output
    local expected

    print_title "Testing job operations"

    expected="One of the main options is mandatory."
    output=$($S9S job --job-id=5 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The main options are mutually exclusive."
    output=$($S9S job --list --log --job-id=5 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the backup handling.
#
function testBackupOperations()
{
    local output
    local expected

    print_title "Testing backup operations"
    expected="One of the --list, --create, --restore and --delete options is mandatory."
    output=$($S9S backup 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The --list, --create, --restore and --delete options are mutually exclusive."
    output=$($S9S backup --list --delete 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The cluster ID or the cluster name must be specified."
    output=$($S9S backup --create 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when cluster id/name missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the cluster handling.
#
function testClusterOperations()
{
    local output
    local expected

    print_title "Testing cluster operations"
    expected="One of the main options is mandatory."
    output=$($S9S cluster 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The main options are mutually exclusive."
    output=$($S9S cluster --list --start 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the cluster handling.
#
function testNodeOperations()
{
    local output
    local expected

    print_title "Testing node operations"
    expected="One main option is required."
    output=$($S9S node 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="Main command line options are mutually exclusive."
    output=$($S9S node --list --set 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message mismatch when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    expected=$'Node list is empty while setting node.\nUse the --nodes command line option to provide the node list.'
    output=$($S9S node --set 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message mismatch when setting unknown host"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected=$'Properties not provided while setting node.\nUse the --properties command line option to provide properties.'
    output=$($S9S node --set --nodes=128.1.1.2 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message mismatch when setting unknown host properties."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the user handling.
#
function testUserOperations()
{
    local output
    local expected

    print_title "Testing user operations"
    expected="One of the main options is mandatory."
    output=$($S9S user 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The main options are mutually exclusive."
    output=$($S9S user --list --create 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the maintenance handling.
#
function testMaintenanceOperations()
{
    local output
    local expected

    expected="One of the --list, --create and --delete options is mandatory."
    output=$($S9S maintenance 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The --list, --create and --delete options are mutually exclusive."
    output=$($S9S maintenance --list --create 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected=$'Missing UUID.\nUse the --uuid command line option to provide the UUID.'
    output=$($S9S maintenance --delete 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when UUID is missing"
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the process handling.
#
function testProcessOperations()
{
    local output
    local expected

    expected="One of the --list and --top options is mandatory."
    output=$($S9S process 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The --list and --top options are mutually exclusive."
    output=$($S9S process --list --top 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi

    return 0
}

#
# This test will pass an invalid url as option argument for the --nodes command
# line option and check the error string.
#
function testInvalidUrl()
{
    output=$($S9S cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="10.10.1.100:" \
        --cluster-name=name \
        --db-admin=postmaster \
        --db-admin-passwd=pwd \
        --provider-version=9.3 \
        2>&1)

    if [ $? -eq 0 ]; then
        failure "Return value is true when passing invalid url."
        return 1
    fi

    if [[ ! "$output" =~ "Expected" ]]; then
        failure "Error string is not what it is expected."
        return 1
    fi

    return 0
}

#
# Running the requested tests.
#
startTests
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testJobOperations
    runFunctionalTest testBackupOperations
    runFunctionalTest testClusterOperations
    runFunctionalTest testNodeOperations
    runFunctionalTest testUserOperations
    runFunctionalTest testMaintenanceOperations
    runFunctionalTest testProcessOperations
    runFunctionalTest testInvalidUrl
fi

endTests


