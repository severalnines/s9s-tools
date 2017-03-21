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

 -h, --help      Print this help and exit.
 --verbose       Print more messages.
 --print-json    Print the JSON messages sent and received.
 --log           Print the logs while waiting for the job to be ended.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log" \
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

CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

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
# Just a normal createUser call we do all the time to register a user on the
# controller so that we can actually execute RPC calls.
#
function testGrantUser()
{
    $S9S user \
        --create \
        --cmon-user=$USER \
        --generate-key \
        $OPTION_PRINT_JSON \
        $OPTION_VERBOSE \
        --batch

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while granting user."
        return 1
    fi

    return 0
}

#
# Checks the main operation command line options for the job handling.
#
function testJobOperations()
{
    local output
    local expected

    expected="One of the --list, --log and --wait options is mandatory."
    output=$($S9S job --job-id=5 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The --list, --log and --wait options are mutually exclusive."
    output=$($S9S job --list --log --job-id=5 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
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

    expected="One of the following options is mandatory: --list, --stat, --create, --ping, --rolling-restart, --add-node, --remove-node, --drop, --stop, --start, --create-account, --delete-account, --create-database, --grant."
    output=$($S9S cluster 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The following options are mutually exclusive: --list, --stat, --create, --ping, --rolling-restart, --add-node, --remove-node, --drop, --stop, --start, --create-account, --delete-account, --create-database, --grant."
    output=$($S9S cluster --list --start 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
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

    expected="One of the --list, --list-config, --change-config, --stat and --set options is mandatory."
    output=$($S9S node 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The --list, --list-config, --change-config, --stat and --set options are mutually exclusive."
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

    expected="One of the --list, --whoami and --create options is mandatory."
    output=$($S9S user 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
        failure "expected : '$expected'"
        failure "output   : '$output'"
        return 1
    fi
    
    expected="The --list, --whoami and --create options are mutually exclusive."
    output=$($S9S user --list --create 2>&1)
    if [ "$output" != "$expected" ]; then
        failure "Error message not as expected when operation is missing."
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

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testGrantUser
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


