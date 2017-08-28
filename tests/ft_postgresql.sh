#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
ALL_CREATED_IPS=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")

# This is the name of the server that will hold the linux containers.
CONTAINER_SERVER="core1"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests various features on PostgreSql. 

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.

EXAMPLES
  Just quickly create a PostgreSQL cluster and add two slave nodes to it
  together with the first node that is going to be the master:

  ./ft_postgresql.sh --log --print-commands testCreateCluster testAddNode testAddNode


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
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    pip-say "The test to create PostgreSQL cluster is starting now."
    nodeName=$(create_node)
    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName
    ALL_CREATED_IPS+=" $nodeName"
    
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
        --provider-version="9.3" \
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

    #s9s node \
    #    --set \
    #    --nodes="$nodes" \
    #    --properties="hostgrouppath=$CONTAINER_SERVER"

}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local exitCode

    pip-say "The test to add node is starting now."
    printVerbose "Creating node..."

    LAST_ADDED_NODE=$(create_node)
    ALL_CREATED_IPS+=" $LAST_ADDED_NODE"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE?slave" \
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
function testStopStartNode()
{
    local exitCode
    local state 

    #
    # First stop the node.
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
    
    state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    if [ "$state" != "DEGRADED" ]; then
        failure "The cluster should be in 'DEGRADED' state, it is '$state'."
    fi

    #
    # Then start.
    #
    mys9s node \
        --start \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    if [ "$state" != "STARTED" ]; then
        failure "The cluster should be in 'STARTED' state, it is '$state'."
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

    pip-say "The test to check configuration is starting now."

    #
    # Listing the configuration values. The exit code should be 0.
    #
    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        >/dev/null

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    #
    # Changing a configuration value.
    # /etc/postgresql/9.6/main/postgresql.conf
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=log_line_prefix \
        --opt-value="'%m'"
    
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
            --opt-name=log_line_prefix \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    #if [ "$value" != "'%m'" ]; then
    #    failure "Configuration value should not be '$value'"
    #fi

    #
    # Pulling a configuration file from a node to the local host.
    #
    rm -rf tmp
    mys9s node \
        --pull-config \
        --nodes=$FIRST_ADDED_NODE \
        --output-dir=tmp \
        --batch

    if [ ! -f "tmp/postgresql.conf" ]; then
        failure "The config file 'tmp/postgresql.conf' was not created."
    fi

    rm -rf tmp

    #
    # This is just to produce some nice output for the user.
    #
    #$S9S node \
    #    --list-config \
    #    --nodes=$FIRST_ADDED_NODE 
}

#
# Creating a new account on the cluster.
#
#
function testCreateAccount()
{
    pip-say "Testing account creation."

    #
    # This command will create a new account on the cluster.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="joe:password" \
        --with-database \
        --batch 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating an account."
    fi
    
    #
    # This command will delete the same account from the cluster.
    # FIXME: this won't work maybe because the user has a database.
    #
    mys9s account \
        --delete \
        --cluster-id=$CLUSTER_ID \
        --account="joe" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    #if [ "$exitCode" -ne 0 ]; then
    #    failure "Exit code is not 0 while deleting an account."
    #fi
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    pip-say "Testing database creation."

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
        failure "Exit code is not 0 while creating a database."
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
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating account."
    fi
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testCreateDatabase.*:DELETE,TRUNCATE" \
        --batch 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while granting privileges."
    fi
}

#
# This will create a backup.
#
function testCreateBackup()
{
    local exitCode
    
    pip-say "The test to create a backup is starting."

    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating a backup."
    fi
    
    #
    # Creating a backup using the cluster name.
    #
    mys9s backup \
        --create \
        --cluster-name=$CLUSTER_NAME \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating a backup"
    fi

    # s9s backup --list --long
    # s9s backup --list --verbose --print-json
}

#
# This will restore a backup. 
#
function testRestoreBackup()
{
    local exitCode
    local backupId

    pip-say "The test to restore a backup is starting."
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        awk '{print $1}')

    #
    # Calling for a rolling restart.
    #
    mys9s backup \
        --restore \
        --cluster-id=$CLUSTER_ID \
        --backup-id=$backupId \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This will remove a backup. 
#
function testRemoveBackup()
{
    local exitCode
    local backupId

    pip-say "The test to remove a backup is starting."
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        awk '{print $1}')

    #
    # Calling for a rolling restart.
    #
    mys9s backup \
        --delete \
        --backup-id=$backupId \
        --batch \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This will remove a backup. 
#
function testRunScript()
{
    local output_file=$(mktemp)
    local exitCode
    local backupId

    pip-say "The test to run scripts is starting."
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        awk '{print $1}')

    #
    # Calling for a rolling restart.
    #
    mys9s script \
        --cluster-id=$CLUSTER_ID \
        --execute scripts/test_output_lines.js \
        2>&1 >$output_file
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    if ! grep --quiet "This is a warning" $output_file; then
        failure "Script output is not as expected."
        echo "output: "
        cat $output_file
    fi

    rm $output_file
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local exitCode
    
    pip-say "The test of rolling restart is starting now."

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
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    pip-say "The test to drop the cluster is starting now."

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
# This will destroy the containers we created.
#
function testDestroyNodes()
{
    pip-say "The test is now destroying the nodes."
    pip-container-destroy \
        --server=$CONTAINER_SERVER \
        $ALL_CREATED_IPS \
        >/dev/null 2>/dev/null
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
    runFunctionalTest testStopStartNode

    runFunctionalTest testConfig

    runFunctionalTest testCreateAccount
    runFunctionalTest testCreateDatabase

    runFunctionalTest testCreateBackup
    runFunctionalTest testRestoreBackup
    runFunctionalTest testRemoveBackup
    
    runFunctionalTest testRunScript
    runFunctionalTest testRollingRestart

    runFunctionalTest testDrop
    runFunctionalTest testDestroyNodes
fi

if [ "$FAILED" == "no" ]; then
    pip-say "The test script is now finished. No errors were detected."
else
    pip-say "The test script is now finished. Some failures were detected."
fi

endTests


