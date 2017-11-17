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

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}' 2>/dev/null)

if [ -z "$PIP_CONTAINER_CREATE" ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi


#
#
#
function testPing()
{
    pip-say "Pinging controller."

    #
    # Pinging. 
    #
    mys9s cluster --ping 

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
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    pip-say "The test to create My SQL replication cluster is starting now."
    nodeName=$(create_node)
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    ALL_CREATED_IPS+=" $nodeName"
    
    nodeName=$(create_node)
    SECOND_ADDED_NODE=$nodeName
    nodes+="$nodeName;"
    ALL_CREATED_IPS+=" $nodeName"
    
    nodeName=$(create_node)
    nodes+="$nodeName"
    ALL_CREATED_IPS+=" $nodeName"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=mysqlreplication \
        --nodes="$nodes" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=5.6 \
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
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
}

#
# This function will check the basic getconfig/setconfig features that reads the
# configuration of one node.
#
function testConfig()
{
    local exitCode
    local value
    local newValue="200"
    local name="max_connections"

    print_title "Changing configuration '$name'"

    #
    # Listing the configuration values. The exit code should be 0.
    #
    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        max_*

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
        exit 1
    fi

    #
    # Changing a configuration value.
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=$name \
        --opt-group=MYSQLD \
        --opt-value=$newValue
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    if [ "$value" != "$newValue" ]; then
        failure "Configuration value should be $newValue not $value"
    fi

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        $name
}

#
# This test will set a configuration value that contains an SI prefixum,
# ("54M").
#
function testSetConfig()
{
    local exitCode
    local value
    local newValue="64M"
    local name="max_heap_table_size"

    print_title "Changing configuration '$name'"

    #
    # Changing a configuration value.
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=$name \
        --opt-group=MYSQLD \
        --opt-value=$newValue
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    if [ "$value" != "$newValue" ]; then
        failure "Configuration value should be $newValue not $value"
    fi

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        'max*'
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

    print_title "Stopping&starting node"

    #
    # First stop.
    # FIXME: Restarting the first node won't work, but what about the second?
    #
    mys9s node \
        --stop \
        --force \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
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
        --nodes=$FIRST_ADDED_NODE \
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
    print_title "Creating account"

    mys9s node --list --long 

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
        failure "Exit code is not 0 while creating an account."
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
    
    LAST_ADDED_NODE=$(create_node)
    nodes+="$LAST_ADDED_NODE"
    ALL_CREATED_IPS+=" $LAST_ADDED_NODE"

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
    fi
}

#
# This test will add one new node to the cluster.
#
function testAddMaster()
{
    local nodes
    local exitCode

    print_title "Adding a master node"

    LAST_ADDED_NODE=$(create_node)
    nodes+="$LAST_ADDED_NODE?master"
    ALL_CREATED_IPS+=" $LAST_ADDED_NODE"

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
    fi
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
    fi
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local exitCode
    
    print_title "Rolling restart"

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
    runFunctionalTest testConfig
    runFunctionalTest testSetConfig

    runFunctionalTest testRestartNode
    runFunctionalTest testStopStartNode

    runFunctionalTest testCreateAccount
    runFunctionalTest testAddNode
    runFunctionalTest testAddMaster
    runFunctionalTest testRemoveNode
    runFunctionalTest testRollingRestart
    runFunctionalTest testStop
    runFunctionalTest testDrop
fi

endTests


