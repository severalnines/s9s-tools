#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""

CONTAINER_SERVER="server1"
PIP_CONTAINER_CREATE=$(which "pip-container-create")

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

function testCreateCluster
{
    local nodes
    local nodeName
    local exitCode

    printVerbose "Creating nodes..."
    nodeName=$(create_node)
    nodes+="mysql://$nodeName;ndb_mgmd://$nodeName;"
    
    nodeName=$(create_node)
    nodes+="mysql://$nodeName;ndb_mgmd://$nodeName;"
    
    nodeName=$(create_node)
    nodes+="ndbd://$nodeName;"
    
    nodeName=$(create_node)
    nodes+="ndbd://$nodeName"

    echo "*** nodes: $nodes"
    $S9S cluster \
        --create \
        --cluster-type=ndb \
        --nodes="$nodes" \
        --vendor=oracle \
        --provider-version=5.6 \
        --log

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
fi

endTests


