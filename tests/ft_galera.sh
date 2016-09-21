#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""

CONTAINER_SERVER="server1"
CLUSTER_NAME="${MYNAME}_$$"
PIP_CONTAINER_CREATE=$(which "pip-container-create")

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

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
    
    nodeName=$(create_node)
    nodes+="$nodeName;"
    
    nodeName=$(create_node)
    nodes+="$nodeName"
    
    echo "Creating cluster"
    $S9S cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor=percona \
        --cluster_name="$CLUSTER_NAME" \
        --provider-version=5.6 \
        --wait

    exitCode=$?
    printVerbose "exitCode = $exitCode"
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
        --cluster-id=1 \
        --nodes="$nodes" \
        --wait
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
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
        --cluster-id=1 \
        --nodes="$LAST_ADDED_NODE" \
        --wait
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
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
        --cluster-id=1 \
        --wait
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
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


