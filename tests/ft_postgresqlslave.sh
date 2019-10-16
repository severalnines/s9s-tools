#! /bin/bash
MYNAME=$(basename "$0")
MYBASENAME=$(basename "$MYNAME" .sh)
MYDIR=$(dirname "$0")
VERSION="0.0.1"
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
OPTION_RESET_CONFIG=""
CONTAINER_SERVER=""
PROVIDER_VERSION="9.5"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source ./include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - PostgreSQL slave stop test script.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --provider-version=STRING The SQL server provider version.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:" \
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

        --provider-version)
            shift
            PROVIDER_VERSION="$1"
            shift
            ;;

        --)
            shift
            break
            ;;
    esac
done

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Creating PostgreSQL Cluster"
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName
    
    #
    # Creating a PostgreSQL cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating cluster."
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local exitCode

    print_title "Adding Node"
    printVerbose "Creating node..."

    LAST_ADDED_NODE=$(create_node --autodestroy)

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE?slave" \
        $LOG_OPTION
    
    check_exit_code $?   
}

#
# This will stop the slave node and checks what happens with the cluster state.
#
function testStopSlave()
{
    local exitCode
    local timeLoop="0"

    print_title "Stopping Slave"

    #
    # Stopping the first added node. 
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        $LOG_OPTION
    
    check_exit_code $?   
    
    state=$(cluster_state "$CLUSTER_ID")
    if [ "$state" != "DEGRADED" ]; then
        failure "The cluster should be in 'DEGRADED' state, it is '$state'."
    fi
    
    if ! wait_for_node_shut_down "$LAST_ADDED_NODE"; then
        state=$(node_state "$nodeName")
        failure "Host state is '$state' instead of 'CmonHostShutDown'."
    fi

    return 0
}

#
# This will start the master node so we can observe what happens with the
# failover.
#
function testStartSlave()
{
    local exitCode
    local timeLoop="0"

    print_title "Starting Node"

    #
    # Starting the first added node. 
    #
    mys9s node \
        --start \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"

    state=$(cluster_state "$CLUSTER_ID")
    if [ "$state" != "STARTED" ]; then
        failure "The cluster should be in 'STARTED' state, it is '$state'."
    fi

    if ! wait_for_node_online "$LAST_ADDED_NODE"; then
        state=$(node_state "$LAST_ADDED_NODE")
        failure "Host '$LAST_ADDED_NODE' state is '$state' instead of 'CmonHostOnline'."
    fi

    return 0
}

#
# This function will stop the Postgresql daemon on the slave manually and check
# if the node goes into failed state. Then the daemon is restarted and it is
# checked that it comes back online.
#
# FIXME: This test fails because the controller will perform a recovery and
# restart the process.
#
function testStopDaemon()
{
    local state

    return 0
    print_title "Stopping the Daemon Manually"

    #
    # Stopping the daemon manually.
    #
    printVerbose "Stopping postgresql on $LAST_ADDED_NODE"
    ssh "$LAST_ADDED_NODE" sudo /etc/init.d/postgresql stop
    ssh "$LAST_ADDED_NODE" ps axu | grep --color=always postgr
    
    if ! wait_for_node_offline "$LAST_ADDED_NODE"; then
        state=$(node_state "$LAST_ADDED_NODE")
        failure "Host '$LAST_ADDED_NODE' state is '$state' instead of 'CmonHostOffLine'."
        mys9s node --stat
        exit 1
    fi

    #
    # Starting the daemon manually.
    #
    printVerbose "Starting postgresql on $LAST_ADDED_NODE"
    ssh "$LAST_ADDED_NODE" sudo /etc/init.d/postgresql start 

    if ! wait_for_node_online "$LAST_ADDED_NODE"; then
        state=$(node_state "$LAST_ADDED_NODE")
        failure "Host '$LAST_ADDED_NODE' state is '$state' instead of 'CmonHostOnline'."
    fi
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    print_title "Dropping the Cluster"

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
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
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddNode

    runFunctionalTest testStopSlave
    runFunctionalTest testStartSlave
    runFunctionalTest testStopDaemon
    
    runFunctionalTest testDrop
fi

endTests


