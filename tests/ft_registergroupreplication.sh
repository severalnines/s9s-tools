#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
ALL_CREATED_IPS=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

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

  $MYNAME - Check for registering group replication clusters.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,reset-config" \
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

        --log)
            shift
            LOG_OPTION="--log"
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
    echo "The s9s program is not installed."
    exit 7
fi

reset_config

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}' 2>/dev/null)

if [ -z "$PIP_CONTAINER_CREATE" ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodeName
    local exitCode

    pip-say "The test to create MySQL replication cluster is starting now."
    nodeName=$(create_node)
    NODES+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    ALL_CREATED_IPS+=" $nodeName"
    
    nodeName=$(create_node)
    NODES+="$nodeName;"
    ALL_CREATED_IPS+=" $nodeName"
    
    nodeName=$(create_node)
    NODES+="$nodeName"
    ALL_CREATED_IPS+=" $nodeName"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=group_replication \
        --nodes="$NODES" \
        --vendor=oracle \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=5.7 \
        $LOG_OPTION

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating cluster."
        return 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    printVerbose "CLUSTER_ID: '$CLUSTER_ID'"

    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    echo "testCreateCluster(): "
    s9s cluster --list --long
    s9s node --list --long
    s9s node --list --node-format="%12R %N\n"
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    pip-say "The test to drop the cluster is starting now."

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

function testRegister()
{
    local exitCode 

    pip-say "The test to register a cluster is starting."

    #
    # Registering the cluester that we just created and dropped.
    #
    mys9s cluster \
        --register \
        --cluster-type=group_replication \
        --nodes=$NODES \
        --vendor=percona \
        --cluster-name=my_cluster_$$ \
        $LOG_OPTION

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    echo "testRegister(): "
    s9s cluster --list --long
    s9s node --list --long
    s9s node --list --node-format="%12R %N\n"
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
    runFunctionalTest testCreateCluster
    runFunctionalTest testDrop
    runFunctionalTest testRegister
fi

if [ "$FAILED" == "no" ]; then
    pip-say "The test script is now finished. No errors were found."
else
    pip-say "The test script is now finished. Some failures were detected."
fi

endTests


