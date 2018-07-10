#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="ft_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

nodes=""


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
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=STRING The SQL server provider version.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:,\
vendor:,provider-version:" \
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

        --provider-version)
            shift
            PROVIDER_VERSION="$1"
            shift
            ;;

        --vendor)
            shift
            OPTION_VENDOR="$1"
            shift
            ;;

        --)
            shift
            break
            ;;
    esac
done

#
#
#
function testPing()
{
    print_title "Pinging Controller"

    #
    # Pinging. 
    #
    mys9s cluster --ping 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller"
        exit 1
    fi
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Creating MySQL Replication Cluster"
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
    nodeName=$(create_node --autodestroy)
    SECOND_ADDED_NODE=$nodeName
    nodes+="$nodeName;"
    
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=mysqlreplication \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    exitCode=$?
    check_exit_code $exitCode

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster was not created"
    fi
}

#
# This test will call a --restart on the node.
#
# The problem in mysqlreplication is that this test will try to stop the
# read-write server and that's now really allowed here.
#
function testRestartNode()
{
    local exitCode

    print_title "Restarting node"

    #
    # Restarting a node. 
    #
    mys9s node \
        --restart \
        --force \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This test will first call a --stop then a --start on a node. Pretty basic
# stuff.
#
# The problem in mysqlreplication is that this test will try to stop the
# read-write server and that's now really allowed here.
#
function testStopStartNode()
{
    local exitCode

    print_title "Stopping&starting non-master node"

    mys9s node --list --long 

    #
    # First stop.
    # FIXME: Restarting the first node won't work, but what about the second?
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$SECOND_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
   
    #
    # Then start.
    #
    mys9s node \
        --start \
        --cluster-id=$CLUSTER_ID \
        --nodes=$SECOND_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
        exit 1
    fi

    #for (( q=0; q<10; q++)); do
    #    s9s node --list --long 
    #    s9s job  --list
    #    sleep 10
    #done
}

#
# Creating a new account on the cluster.
#
function testCreateAccount()
{
    local master_hostname

    print_title "Creating Account"

    #
    #
    #
    for waiting in $(seq 1 10); do
        master_hostname=$( \
            s9s node --list --long | \
            grep ^..M | \
            awk '{print $5}')

        if [ "$master_hostname" ]; then
            if s9s node --stat "$master_hostname" | grep --quiet "read-only"
            then
                master_hostname=""
            fi
        fi

        if [ -z "$master_hostname" ]; then
            echo "There seem to be no master host."
            echo "Waiting 20 seconds..."

            sleep 20
            continue
        fi

        echo "This seems to be the master."
        mys9s node --stat $master_hostname
        break
    done

    if [ -z "$master_hostname" ]; then
        echo "There seems to be no master, will continue anyway."
    fi

    #
    # This command will create a new account on the cluster.
    #
    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="john_doe:password@1.2.3.4" \
        --with-database 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not $exitCode while creating an account"
        mys9s node --list --long 
        mys9s node --stat
        exit 1
    fi
}

#
# This test will create an account then immediately delete it.
#
function createDeleteAccount()
{
    print_title "Creating then Deleting Account"

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="tmpaccount:tmppasswd@192.168.0.127"

    check_exit_code_no_job $?
    mys9s account --list --long

    mys9s cluster \
        --delete-account \
        --cluster-id=$CLUSTER_ID \
        --account="tmpaccount@192.168.0.127"

    check_exit_code_no_job $?
    mys9s account --list --long

    if mys9s account --list --long | grep --quiet "tmpaccount"; then
        failure "The 'tmpaccount' account is still there."
    fi
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local nodes
    local exitCode

    print_title "Adding a new node"
    
    LAST_ADDED_NODE=$(create_node --autodestroy)
    nodes+="$LAST_ADDED_NODE"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
        mys9s job --log --job-id=6
        exit 1
    fi
}

#
# This test will add one new node to the cluster.
#
function testAddMaster()
{
    local exitCode

    print_title "Adding a master node"

    LAST_ADDED_NODE=$(create_node --autodestroy)

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE;master" \
        $LOG_OPTION
    
    check_exit_code $? 
}



#
# This test will remove the last added node.
#
function testRemoveNode()
{
    print_title "Removing node"

    if [ -z "$LAST_ADDED_NODE" ]; then
        echo "Skipping test."
    fi
    
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
        mys9s job --log --job-id=6
        exit 1
    fi
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local exitCode
    
    print_title "Rolling restart"
    mys9s node --list --long

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
   
    check_exit_code $?
}

#
# Stopping the cluster.
#
function testStop()
{
    local exitCode

    print_title "Stopping the cluster"

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
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    print_title "Dropping the cluster"

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
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster

    #runFunctionalTest testRestartNode
    #runFunctionalTest testStopStartNode

    runFunctionalTest testCreateAccount
    runFunctionalTest createDeleteAccount
    runFunctionalTest testAddNode
    #runFunctionalTest testAddMaster
    runFunctionalTest testRemoveNode
    runFunctionalTest testRollingRestart
    runFunctionalTest testStop
    runFunctionalTest testDrop
fi

endTests


