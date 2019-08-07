#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
OPTION_NUMBER_OF_NODES="3"
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

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
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --vendor=STRING  Use the given Galera vendor.

  --provider-version=STRING  The SQL server provider version.
  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:
  o testPing             Checks if the controller is on-line.
  o testCreateCluster    Creating a MySQL replication cluster.
  o testCreateDatabase   Creating some databases.
  o testCreateBackup     Creates a backup on the cluster.
  o testCreateAccount    Creates an account.
  o createDeleteAccount  Creates and deletes an account.
  o testAddNode          Adds a new node to the cluster.
  o testRemoveNode       Removes a node from the cluster.
  o testStop             Stops the cluster.
  o testDrop             Drops the cluster.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,install,reset-config,
server:,number-of-nodes:,vendor:,provider-version:" \
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

        --provider-version)
            shift
            PROVIDER_VERSION="$1"
            shift
            ;;

        --number-of-nodes)
            shift
            OPTION_NUMBER_OF_NODES="$1"
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
    local node_ip
    local exitCode
    local node_serial=1
    local node_name

    print_title "Creating MySQL Replication Cluster"
    while [ "$node_serial" -le "$OPTION_NUMBER_OF_NODES" ]; do
        node_name=$(printf "${MYBASENAME}_node%03d_$$" "$node_serial")

        echo "Creating node #$node_serial"
        node_ip=$(create_node --autodestroy "$node_name")

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$node_ip"
        
        case $node_serial in 
            1)
                FIRST_ADDED_NODE="$node_ip"
                ;;

            2)
                SECOND_ADDED_NODE="$node_ip"
                ;;

            3)
                THIRD_ADDED_NODE="$node_ip"
                ;;
        esac

        let node_serial+=1
    done
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --job-tags="create" \
        --cluster-type=mysqlreplication \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster was not created"
    fi

    wait_for_cluster_started "$CLUSTER_NAME" 

    mys9s cluster --stat
    mys9s node    --stat

    #
    # Checking the controller, the nodes and the cluster.
    #
    check_controller \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    
    for node in $(echo "$nodes" | tr ';' ' '); do
        check_node \
            --node       "$node" \
            --ip-address "$node" \
            --port       "3306" \
            --config     "/etc/mysql/my.cnf" \
            --owner      "pipas" \
            --group      "testgroup" \
            --cdt-path   "/$CLUSTER_NAME" \
            --status     "CmonHostOnline" \
            --no-maint
    done

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "REPLICATION" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"

    echo "  FIRST_ADDED_NODE: '$FIRST_ADDED_NODE'"
    echo " SECOND_ADDED_NODE: '$SECOND_ADDED_NODE'"
    echo "  THIRD_ADDED_NODE: '$THIRD_ADDED_NODE'"
    mys9s replication --list --long
}

function testStopStartReplication()
{
    print_title "Stopping and Starting Replication"
    cat <<EOF
  This test will stop then start the replication on a replication slave.

EOF

    mys9s replication --list --long

    mys9s replication \
        --stop \
        --cluster-id=1 \
        --job-tags="stop" \
        --slave=$SECOND_ADDED_NODE:3306 \
        $LOG_OPTION

    check_exit_code $?
    
    mys9s replication --list --long

    mys9s replication \
        --start \
        --cluster-id=1 \
        --job-tags="start" \
        --slave=$SECOND_ADDED_NODE:3306 \
        $LOG_OPTION

    check_exit_code $?
    
    mys9s replication --list --long
}

function testStageSlave()
{
    print_title "Testing the Rebuilding a Replication Slave"
    cat <<EOF
  This test will use the --stage option to rebuild the replication slave and
  then check if the job was properly executed.

EOF

    mys9s replication --list --long

    mys9s replication \
        --stage \
        --cluster-id=1 \
        --job-tags="stage" \
        --slave=$SECOND_ADDED_NODE:3306 \
        --master=$FIRST_ADDED_NODE:3306 \
        $LOG_OPTION
    
    check_exit_code $?
    
    mys9s replication --list --long
    mys9s job --list --long
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local database_name

    print_title "Creating Database"
    cat <<EOF
  This test will create a few databases, check if the database creation reports
  back a success.

EOF

    #
    # This command will create a new database on the cluster.
    #
    for database_name in database01 database02 database03; do
        mys9s cluster \
            --create-database \
            --cluster-id=$CLUSTER_ID \
            --db-name="$database_name" \
            --batch
    
        check_exit_code_no_job $?
    done

    s9s cluster --list-databases --long --cluster-id=$CLUSTER_ID
}

function testCreateBackup()
{
    print_title "Creating Backups"

    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        $LOG_OPTION
    
    check_exit_code $?
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

    print_title "Adding a New Node"
    
    LAST_ADDED_NODE=$(create_node --autodestroy "${MYNAME}_11_$$")
    nodes+="$LAST_ADDED_NODE"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
   
    check_exit_code $?

    mys9s node --list --long
    mys9s cluster --stat
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
    if [ "$exitCode" -ne 0 ]; then
        mys9s job --list 
        check_exit_code $?
    fi
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
        runFunctionalTest testAddNode
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    runFunctionalTest testStopStartReplication
    runFunctionalTest testStageSlave

    runFunctionalTest testCreateDatabase
    runFunctionalTest testCreateBackup

    #runFunctionalTest testRestartNode
    #runFunctionalTest testStopStartNode

    runFunctionalTest testCreateAccount
    runFunctionalTest createDeleteAccount
    runFunctionalTest testAddNode
    #runFunctionalTest testAddMaster
    runFunctionalTest testRemoveNode
    runFunctionalTest testStop
    runFunctionalTest testDrop
fi

endTests


