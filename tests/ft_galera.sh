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
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

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
 
  $MYNAME - Test script for s9s to check Galera clusters.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --vendor=STRING  Use the given Galera vendor.
 --provider-version=STRING The SQL server provider version.
 --leave-nodes    Do not destroy the nodes at exit.

EXAMPLE
 ./ft_galera.sh --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,vendor:,leave-nodes" \
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
        
        --vendor)
            shift
            OPTION_VENDOR="$1"
            shift
            ;;

        --leave-nodes)
            shift
            OPTION_LEAVE_NODES="true"
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

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

if [ -z $(which pip-container-create) ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

#
#
#
function testPing()
{
    print_title "Pinging controller."

    #
    # Pinging. 
    #
    mys9s cluster --ping 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
        #pip-say "The controller is off line. Further testing is not possible."
    #else
        #pip-say "The controller is on line."
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

    print_title "Testing the creation of a Galera cluster"

    echo "Creating node #0"
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
    echo "Creating node #1"
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName;"
    
    echo "Creating node #2"
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName"
    
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
        exit 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
}

#
# This function will check the basic getconfig/setconfig features that reads the
# configuration of one node.
#
function testSetConfig01()
{
    local exitCode
    local value
    local newValue
    local name

    printVerbose "Checking the configuration"

    #
    # Listing the configuration values. The exit code should be 0.
    #
    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    #
    # Changing a configuration value.
    #
    newValue=200
    name="max_connections"
    
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
function testSetConfig02()
{
    local exitCode
    local value
    local newValue="64M"
    local name="max_heap_table_size"

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
function testRestartNode()
{
    local exitCode

    #
    # Restarting a node. 
    #
    mys9s node \
        --restart \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
        mys9s job --log --job-id=4
        mys9s job --log --job-id=5
    fi
}

#
# This test will first call a --stop then a --start on a node. Pretty basic
# stuff.
#
function testStopStartNode()
{
    local exitCode

    #
    # First stop.
    #
    mys9s node \
        --stop \
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
    fi
}

#
# Creating a new account on the cluster.
#
function testCreateAccount()
{
    local userName

    print_title "Testing account creation."

    #
    # This command will create a new account on the cluster.
    #
    if [ -z "$CLUSTER_ID" ]; then
        failure "No cluster ID found."
        return 1
    fi

    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="john_doe:password@1.2.3.4" \
        --with-database
    
    check_exit_code_no_job $?

    #
    # Checking if the account is created.
    #
    userName=$(s9s account --list --cluster-id=1 john_doe)
    if [ "$userName" != "john_doe" ]; then
        failure "Failed to create user 'john_doe'."
        exit 1
    fi

    echo "Before granting."
    mys9s account --list --long --cluster-id=$CLUSTER_ID john_doe
    
    #
    #
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="john_doe@1.2.3.4" \
        --privileges="*.*:ALL" 
    
    check_exit_code_no_job $?

    echo "After granting."
    mys9s account --list --long --cluster-id=$CLUSTER_ID john_doe

    #
    # Dropping the account, checking if it is indeed dropped.
    #
    mys9s account \
        --delete \
        --cluster-id=$CLUSTER_ID \
        --account="john_doe@1.2.3.4"
    
    check_exit_code_no_job $?

    userName=$(s9s account --list --long john_doe --batch)
    if [ "$userName" ]; then
        failure "The account 'john_doe' still exists."
        mys9s account --list --long --cluster-id=$CLUSTER_ID john_doe
        exit 1
    fi

}


#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local userName

    print_title "Testing database creation."

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testCreateDatabase" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        exit 1
    fi
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="pipas:password" \
        --privileges="testCreateDatabase.*:INSERT,UPDATE" \
        --batch
    
    check_exit_code_no_job $?
  
    #
    # Checking if the account could be created.
    #
    userName=$(s9s account --list --cluster-id=1 pipas)
    if [ "$userName" != "pipas" ]; then
        failure "Failed to create user 'pipas'."
        exit 1
    fi

    #
    # This command will grant some rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testCreateDatabase.*:DELETE,TRUNCATE" \
        --batch 
   
    check_exit_code_no_job $?

    mys9s account --list --long
}

#
# This test will create a user and a database and then upload some data if the
# data can be found on the local computer.
#
function testUploadData()
{
    local db_name="pipas1"
    local user_name="pipas1"
    local password="p"
    local reply
    local count=0

    print_title "Testing data upload on cluster."

    #
    # Creating a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --db-name=$db_name
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        exit 1
    fi

    #
    # Creating a new account on the cluster.
    #
    mys9s account \
        --create \
        --account="$user_name:$password" \
        --privileges="$db_name.*:ALL"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        exit 1
    fi

    #
    # Executing a simple SQL statement using the account we created.
    #
    reply=$(\
        mysql \
            --disable-auto-rehash \
            --batch \
            -h$FIRST_ADDED_NODE \
            -u$user_name \
            -p$password \
            $db_name \
            -e "SELECT 41+1" | tail -n +2 )

    if [ "$reply" != "42" ]; then
        failure "Cluster failed to execute an SQL statement: '$reply'."
    fi

    #
    # Here we upload some tables. This part needs test data...
    #
    for file in /home/pipas/Desktop/stuff/databases/*.sql.gz; do
        if [ ! -f "$file" ]; then
            continue
        fi

        printf "%'6d " "$count"
        printf "$XTERM_COLOR_RED$file$TERM_NORMAL"
        printf "\n"
        zcat $file | \
            mysql --batch -h$FIRST_ADDED_NODE -u$user_name -pp $db_name

        exitCode=$?
        if [ "$exitCode" -ne 0 ]; then
            failure "Exit code is $exitCode while uploading data."
            break
        fi

        let count+=1
        if [ "$count" -gt 99 ]; then
            break
        fi
    done
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local nodes

    print_title "Adding a node"

    printVerbose "Creating node..."
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
    
    check_exit_code $?
}

#
# This test will add a proxy sql node.
#
function testAddProxySql()
{
    local node
    local nodes

    print_title "Adding a ProxySQL Node"

    nodeName=$(create_node --autodestroy)
    nodes+="proxySql://$nodeName"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
    
    check_exit_code $?

    mys9s node \
        --list-config \
        --nodes=$nodeName 
}

#
# This test will first add a HaProxy node, then remove from the cluster. The
# idea behind this test is that the remove-node call should be identify the node
# using the IP address if there are multiple nodes with the same IP (one galera
# node and one haproxy node on the same host this time).
#
function testAddRemoveHaProxy()
{
    local node
    local nodes
    
    print_title "Adding and removing HaProxy node"
    mys9s node --list --long 

    node=$(\
        $S9S node --list --long --batch | \
        grep ^g | \
        tail -n1 | \
        awk '{print $5}')

    #
    # Adding a node to the cluster.
    #
    printVerbose "Adding haproxy at '$node'."
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="haProxy://$node" \
        $LOG_OPTION
    
    check_exit_code $?
   
    mys9s node --list --long --color=always

    #
    # Remove a node from the cluster.
    #
    printVerbose "Removing haproxy at '$node:9600'."
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$node:9600" \
        $LOG_OPTION
    
    check_exit_code $?
    
    mys9s node --list --long --color=always
}

#
# This test will add a HaProxy node.
#
function testAddHaProxy()
{
    local node
    local nodes
    
    print_title "Adding a HaProxy Node"

    node=$(create_node --autodestroy)
    nodes+="haProxy://$node"

    #
    # Adding haproxy to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
    
    check_exit_code $?
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
    
    #
    # Removing the last added node.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$LAST_ADDED_NODE" \
        $LOG_OPTION
    
    check_exit_code $?
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    print_title "The test of rolling restart is starting now."

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
# This will create a backup. Here we pass the backup-dir, so this should work
# even if the backup directory is not set in the config.
#
function testCreateBackup()
{
    print_title "The test to create a backup is starting."

    #
    # Creating the backup.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        $LOG_OPTION
    
    check_exit_code $?
}

#
# This will restore a backup. 
#
function testRestoreBackup()
{
    local backupId

    print_title "The test to restore a backup is starting."
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        head -n1 | \
        awk '{print $1}')

    mys9s backup --list --long --batch --cluster-id=$CLUSTER_ID
    
    #
    # Restoring the backup. 
    #
    mys9s backup \
        --restore \
        --cluster-id="$CLUSTER_ID" \
        --backup-id="$backupId" \
        $LOG_OPTION
    
    check_exit_code $?
}

#
# This will remove a backup. 
#
function testRemoveBackup()
{
    local backupId

    print_title "Removing a Backup"

    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        awk '{print $1}')

    #
    # Calling for a rolling restart.
    #
    mys9s backup \
        --delete \
        --backup-id=$backupId \
        $LOG_OPTION
    
    check_exit_code $?
}

#
# Stopping the cluster.
#
function testStop()
{
    print_title "Stopping Cluster"

    #
    # Stopping the cluster.
    #
    mys9s cluster \
        --stop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    check_exit_code $?
}

#
# Starting the cluster.
#
function testStart()
{
    print_title "Starting Cluster"

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --start \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    check_exit_code $?
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

    runFunctionalTest testSetConfig01
    runFunctionalTest testSetConfig02

    runFunctionalTest testRestartNode
    runFunctionalTest testStopStartNode

    runFunctionalTest testCreateAccount
    runFunctionalTest testCreateDatabase

    runFunctionalTest testAddNode
    runFunctionalTest testAddProxySql
    runFunctionalTest testAddRemoveHaProxy
    runFunctionalTest testAddHaProxy
    runFunctionalTest testRemoveNode
    runFunctionalTest testRollingRestart
    runFunctionalTest testCreateBackup
    #runFunctionalTest testRestoreBackup
    runFunctionalTest testRemoveBackup
    runFunctionalTest testStop
    runFunctionalTest testStart
fi

endTests


