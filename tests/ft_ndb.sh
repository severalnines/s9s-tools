#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""

LOG_OPTION="--wait"
DEBUG_OPTION=""
PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""

# This is the name of the server that will hold the linux containers.
CONTAINER_SERVER=""

# The IP of the node we added last. Empty if we did not.
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
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for to check ndb clusters.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o testCreateCluster    Creating a cluster.
  o testCreateBackup     Creating a backup of the cluster.
  o testAddNode          Add a node to the existing cluster.
  o testRemoveNode       Remove a node from the cluster.
  o testRollingRestart   Rolling restart.
  o testDrop             Dropping the cluster from the controller.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,print-commands,reset-config,server:,install" \
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
            DEBUG_OPTION="--debug"
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
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode
    local node_names

    print_title "Creating an NDB Cluster."
    begin_verbatim

    nodeName=$(create_node --autodestroy "${MYBASENAME}_node001_$$")
    node_names+="$nodeName "
    FIRST_ADDED_NODE="$nodeName"
    nodes+="mysql://$nodeName;ndb_mgmd://$nodeName;"

    nodeName=$(create_node --autodestroy "${MYBASENAME}_node002_$$")
    node_names+="$nodeName "
    nodes+="mysql://$nodeName;ndb_mgmd://$nodeName;"
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_node003_$$")
    node_names+="$nodeName "
    nodes+="ndbd://$nodeName;"
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_node004_$$")
    node_names+="$nodeName "
    nodes+="ndbd://$nodeName"

    #
    # Creating an NDB cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=ndb \
        --nodes="$nodes" \
        --vendor=oracle \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        $DEBUG_OPTION \
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

    wait_for_cluster_started "$CLUSTER_NAME" 

    #
    # Checking the controller, the nodes and the cluster.
    #
    check_controller \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
 
    # FIXME: Here is the thing: the nodes are different from each other, so it
    # is hard to check.
#    for node in $(echo "$node_names" | tr ';' ' '); do
#        check_node \
#            --node       "$node" \
#            --ip-address "$node" \
#            --port       "3306" \
#            --config     "/etc/mysql/my.cnf" \
#            --owner      "pipas" \
#            --group      "testgroup" \
#            --cdt-path   "/$CLUSTER_NAME" \
#            --status     "CmonHostOnline" \
#            --no-maint
#    done

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "MYSQLCLUSTER" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"

    end_verbatim
}

function testCreateBackup()
{
    print_title "Creating Backups"
    begin_verbatim

    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE:3306" \
        --backup-directory=/tmp \
        $DEBUG_OPTION \
        $LOG_OPTION
    
    check_exit_code $?
    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local nodes
    local exitCode

    print_title "Adding Node to Cluster"
    begin_verbatim

    printVerbose "Creating Node..."
    nodeName=$(create_node --autodestroy)
    LAST_ADDED_NODE=$nodeName
    nodes+="$nodeName"

    #
    # Adding a node to the cluster.
    # We can do this by cluster ID and we also can do this by cluster name, but
    # it takes a bit too long to test both.
    #
    mys9s cluster \
        --add-node \
        --cluster-name=$CLUSTER_NAME \
        --nodes="$nodes" \
        $DEBUG_OPTION \
        $LOG_OPTION
   
    check_exit_code $?

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
    
    print_title "The test to remove node is starting now."
    begin_verbatim

    #
    # Removing the last added node.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$LAST_ADDED_NODE" \
        $DEBUG_OPTION \
        $LOG_OPTION
    
    check_exit_code $?

    end_verbatim
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    print_title "The test of rolling restart is starting now."
    begin_verbatim

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $DEBUG_OPTION \
        $LOG_OPTION
   
    check_exit_code $?
    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    print_title "The test to drop the cluster is starting now."
    begin_verbatim

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $DEBUG_OPTION \
        $LOG_OPTION
    
    check_exit_code $?

    end_verbatim
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    if [ -n "$*" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    #runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateBackup
    runFunctionalTest testAddNode
    runFunctionalTest testRemoveNode
    runFunctionalTest testRollingRestart
    runFunctionalTest testDrop
fi

endTests


