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
CONTAINER_SERVER=""

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME3="${MYBASENAME}_13_$$"

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests group replication clusters.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers. 
  --print-commands Do not print unit test info, print the executed commands.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config" \
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

        --server)
            shift
            CONTAINER_SERVER="$1"
            shift
            ;;

        --print-commands)
            shift
            DONT_PRINT_TEST_MESSAGES="true"
            PRINT_COMMANDS="true"
            ;;

        --install)
            shift
            OPTION_INSTALL="--install"
            ;;

        --reset-config)
            shift
            OPTION_RESET_CONFIG="true"
            ;;

        --)
            shift
            break
            ;;
    esac
done

CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

#if [ ! -d data -a -d tests/data ]; then
#    echo "Entering directory tests..."
#    cd tests
#fi
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
    local nodes
    local nodeName
    local exitCode

    print_title "Creating MySQL Replication Cluster"
    cat <<EOF | grep paragraph
  This test will create some containers and install a group replication cluster
  on top of them.
EOF

    begin_verbatim

    nodeName=$(create_node --autodestroy "$CONTAINER_NAME1")
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
    nodeName=$(create_node --autodestroy "$CONTAINER_NAME2")
    nodes+="$nodeName;"
    
    nodeName=$(create_node --autodestroy "$CONTAINER_NAME3")
    nodes+="$nodeName"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=group_replication \
        --nodes="$nodes" \
        --vendor=oracle \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=5.7 \
        $LOG_OPTION

    check_exit_code $?
    
    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" == "NOT-FOUND" ]; then
        failure "Cluster was not created."
        end_verbatim
        return 1
    elif [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
    
    end_verbatim
}

#
# 
#
function testConfig()
{
    local exitCode
    
    print_title "Checking Configuration"
    begin_verbatim

    #
    # Listing the configuration values.
    #
    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE:3306 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local nodeName
    local nodes
    local exitCode

    print_title "Adding Node"
    begin_verbatim

    printVerbose "Creating node..."
    nodeName=$(create_node --autodestroy)
    LAST_ADDED_NODE="$nodeName"
    nodes+="$nodeName"
    
    printVerbose "Created node '$LAST_ADDED_NODE'."
    printVerbose "*** nodes: '$nodes'"

    #
    # Adding a node to the cluster.
    #
    printVerbose "Adding node:"
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    end_verbatim
}

#
# This test will remove the last added node.
#
function testRemoveNode()
{
    if [ -z "$LAST_ADDED_NODE" ]; then
        printVerbose "Skipping test."
    fi
    
    print_title "Removing Node"
    begin_verbatim
    
    #
    # Removing the last added node.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$LAST_ADDED_NODE" \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    end_verbatim
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local exitCode
    
    print_title "Rolling Restart"
    begin_verbatim

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    end_verbatim
}

#
# Stopping the cluster.
#
function testStop()
{
    local exitCode

    print_title "Stopping Cluster"
    begin_verbatim

    #
    # Stopping the cluster.
    #
    mys9s cluster \
        --stop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    print_title "Dropping the Cluster"
    begin_verbatim

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

    end_verbatim
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testCreateCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    runFunctionalTest testConfig

    #runFunctionalTest testAddNode
    #runFunctionalTest testRemoveNode
    #runFunctionalTest testRollingRestart
    #runFunctionalTest testStop
    runFunctionalTest testDrop
fi

endTests


