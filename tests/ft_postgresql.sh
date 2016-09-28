#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CONTAINER_SERVER="server1"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh

#
# Prints an error message to the standard error. The text will not mixed up with
# the data that is printed to the standard output.
#
function printError()
{
    local datestring=$(date "+%Y-%m-%d %H:%M:%S")

    echo -e "$MYNAME($$) $*" >&2

    if [ "$LOGFILE" ]; then
        echo -e "$datestring ERROR $MYNAME($$) $*" >>"$LOGFILE"
    fi
}

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
 --log           Print the logs while waiting for the job to be ended.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log" \
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
if [ -z "$PIP_CONTAINER_CREATE" ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

function create_node()
{
    $PIP_CONTAINER_CREATE --server=$CONTAINER_SERVER
}

#
# $1: the name of the cluster
#
function find_cluster_id()
{
    local clusterName="$1"
    local retval

    retval=$(s9s cluster --list --long --batch --cluster-name="$clusterName" | awk '{print $1}')
    if [ -z "$retval" ]; then
        printError "Cluster '$clusterName' was not found."
    else
        printVerbose "Cluster '$clusterName' was found with ID ${retval}."
    fi

    echo $retval
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster
{
    local nodes
    local nodeName
    local exitCode

    echo "Creating nodes..."
    nodeName=$(create_node)
    nodes+="$nodeName;"
    
    echo "Creating cluster"
    $S9S cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        $LOG_OPTION

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating cluster."
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid."
    fi
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local nodes
    local exitCode

    printVerbose "Creating Node..."
    LAST_ADDED_NODE=$(create_node)
    nodes+="$LAST_ADDED_NODE"

    echo "Adding Node"
    $S9S cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}."
    fi
}

#
# This test will remove the last added node.
#
function testRemoveNode()
{
    if [ -z "$LAST_ADDED_NODE" ]; then
        printVerbose "Skipping test."
    fi
    
    printVerbose "Removing Node"
    $S9S cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$LAST_ADDED_NODE" \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}."
    fi
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local exitCode

    echo "Performing Rolling Restart"
    $S9S cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}."
    fi
}

#
# Running the requested tests.
#
startTests

if [ "$1" ]; then
    runFunctionalTest "$1"
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddNode
    runFunctionalTest testRemoveNode
    runFunctionalTest testRollingRestart
fi

endTests


