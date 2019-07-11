#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.4"

LOG_OPTION="--wait"
DEBUG_OPTION=""

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""
PROVIDER_VERSION="10"

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

 -h, --help          Print this help and exit.
 --verbose           Print more messages.
 --log               Print the logs while waiting for the job to be ended.
 --server=SERVER     The name of the server that will hold the containers.
 --print-commands    Do not print unit test info, print the executed commands.
 --install           Just install the cluster and exit.
 --reset-config      Remove and re-generate the ~/.s9s directory.
 
 --provider-version=STRING The SQL server provider version.

SUPPORTED TESTS:
  o testCreateCluster    Creating a PostgreSql cluster.
  o testAddNode          Adds a slave to the cluster.
  o testStopStartNode    Stopping then starting a node.
  o testConfig           Reading and changing the configuration.
  o testConfigFail       Testing configuration changes that should fail.
  o testCreateAccount01  Create an account on the cluster.
  o testCreateAccount02  Create more accounts on the cluster.
  o testCreateDatabase   Creates database and account.
  o testCreateBackup     Creates a backup.
  o testRestoreBackup    Restores a backup.
  o testRemoveBackup     Removes a backup.
  o testRunScript        Runs a JS script on the cluster.
  o testRollingRestart   Rolling restart test.
  o testDrop             Drops the previously created cluster.

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
            DEBUG_OPTION="--debug"
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
    local node1="ft_postgresql_01_$$"
    local nodes
    local nodeName

    print_title "Creating a PostgreSQL Cluster"
    
    for server in $(echo $CONTAINER_SERVER | tr ',' ' '); do
        [ "$servers" ] && servers+=";"
        servers+="lxc://$server"
    done

    if [ "$servers" ]; then
        mys9s server --register --servers=$servers
    fi

    #
    # Creating containers.
    #
    nodeName=$(create_node --autodestroy $node1)
    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName
  
    #
    # Check the nodes before creating a cluster.
    #
#    mys9s cluster \
#        --check-hosts \
#        --nodes="$nodes" 
#
#    check_exit_code_no_job $?
#    
#    mys9s cluster \
#        --check-hosts \
#        --nodes="$nodes" \
#        --print-json
#    
#    check_exit_code_no_job $?

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
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    
    #
    #
    #
    print_title "Waiting until the Cluster Started"
    wait_for_cluster_started "$CLUSTER_NAME"
    if [ $? -ne 0 ]; then
        failure "The cluster failed to start."
        mys9s cluster --stat
    fi


    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
        exit 1
    fi

    mys9s cluster --stat
    mys9s node    --stat

    #
    # Checking the controller, the node and the cluster.
    #
    check_controller \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    
    check_node \
        --node       "$FIRST_ADDED_NODE" \
        --ip-address "$FIRST_ADDED_NODE" \
        --port       "8089" \
        --config     "/etc/postgresql/$PROVIDER_VERSION/main/postgresql.conf" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "POSTGRESQL_SINGLE" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    print_title "Adding a New Node"
    cat <<EOF
This test will add a new node as slave to the cluster created in the previous
test as a single node postgresql cluster.

EOF

    LAST_ADDED_NODE=$(create_node --autodestroy)

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE?slave" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $? 

    check_node \
        --node       "$LAST_ADDED_NODE" \
        --ip-address "$LAST_ADDED_NODE" \
        --port       "5432" \
        --config     "/etc/postgresql/$PROVIDER_VERSION/main/postgresql.conf" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint
}

#
# This test will first call a --stop then a --start on a node. Pretty basic
# stuff.
#
function testStopStartNode()
{
    local state 

    print_title "Stopping and Starting a Node"

    #
    # First stop the node.
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
    
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
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    

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

    print_title "Checking Configuration"

    #
    # Listing the configuration values. The exit code should be 0.
    #
    mys9s node --stat "$FIRST_ADDED_NODE"

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
        failure "Failed to pull the configuration."
    else
        success "  o File 'tmp/postgresql.conf' is found, ok."
    fi

    rm -rf tmp

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE 
}

function testConfigFail()
{
    local exitCode
    local value

    print_title "Checking Configuration with Failed Settings"
    cat <<EOF
This test will check some use-cases where the --change-config should fail. We
send a string instead of an integer and we also check what happens when an 
invalid/non-existing hostname is send (we had a crash).

EOF

    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=max_connections \
        --opt-value="notaninteger"
    
    exitCode=$?
    if [ "$exitCode" -eq 0 ]; then
        failure "Changing 'max_connections' to non-integer should fail."
        mys9s node \
            --list-config \
            --nodes=$FIRST_ADDED_NODE 
    else
        success "  o Request failed, ok."
    fi
    
    mys9s node \
        --change-config \
        --nodes=10.10.1.100 \
        --opt-name=max_connections \
        --opt-value="500"
    
    exitCode=$?
    if [ "$exitCode" -eq 0 ]; then
        failure "Request with non-existing node should have failed."
        mys9s node \
            --list-config \
            --nodes=$FIRST_ADDED_NODE 
    else
        success "  o Request failed, ok."
    fi
}

#
# Creating a new account on the cluster.
#
#
function testCreateAccount01()
{
    print_title "Creating an Account"

    #
    # This command will create a new account on the cluster.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="joe:password" 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating an account"
        mys9s node --stat
    fi
    
    #
    # This command will delete the same account from the cluster.
    # FIXME: this won't work maybe because the user has a database.
    #
    mys9s account \
        --delete \
        --cluster-id=$CLUSTER_ID \
        --account="joe" 
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not $exitCode while deleting an account"
    fi

    mys9s account --list --long
}

function testCreateAccount02()
{
    print_title "Create Accounts"

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account='jake:jake@192.168.0.0/24' \
        --print-json \
        --verbose 

    mys9s account --list --long
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    print_title "Creating Databases"

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testCreateDatabase"
   
    check_exit_code_no_job $?
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="pipas:password" \
        --privileges="testCreateDatabase.*:INSERT,UPDATE"
    
    check_exit_code_no_job $?
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testCreateDatabase.*:DELETE" \
        --batch 
    
    check_exit_code_no_job $?
}

#
# This will create a backup.
#
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
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    
    #
    # Creating a backup using the cluster name. It is a pg_basebackup this 
    # time.
    #
    mys9s backup \
        --create \
        --cluster-name=$CLUSTER_NAME \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        --backup-method=pg_basebackup \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    

    mys9s backup --list --long
   

    #
    # Creating a backup using the pgbackrestfull method.
    # Well, it doesn't work...
    #
#    mys9s backup \
#        --create \
#        --cluster-name=$CLUSTER_NAME \
#        --nodes=$FIRST_ADDED_NODE \
#        --backup-directory=/tmp \
#        --backup-method=pgbackrestfull \
#        $LOG_OPTION
#    
#    check_exit_code $?    
#
#    mys9s backup --list --long
}

#
# This will restore a backup. 
#
function testRestoreBackup()
{
    local backupId
    local exitCode

    print_title "Restoring a Backup"

    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        head -n1 | \
        awk '{print $1}')

    #
    # Restoring the backup. 
    #
    mys9s backup \
        --restore \
        --cluster-id=$CLUSTER_ID \
        --backup-id=$backupId \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    check_exit_code $exitCode

    if [ "$exitCode" -ne 0 ]; then
        exit 1
    fi
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
        head -n1 | \
        awk '{print $1}')

    #
    # Removing the backup. 
    #
    mys9s backup \
        --delete \
        --backup-id=$backupId \
        --batch \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
}

#
# 
#
function testRunScript()
{
    local output_file=$(mktemp)
    local script_file="scripts/test_output_lines.js"
    local exitCode
    
    print_title "Running a Script"
    
    cat $script_file

    #
    # Running a script. 
    #
    mys9s script \
        --cluster-id=$CLUSTER_ID \
        --execute "$script_file" \
        2>&1 >$output_file
    
    exitCode=$?

    echo "output: "
    cat "$output_file"

    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
        rm $output_file
        exit 1
    fi

    if ! grep --quiet "This is a warning" $output_file; then
        failure "Script output is not as expected."
        rm $output_file
        exit 1
    fi

    rm $output_file
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    print_title "Performing Rolling Restart"

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    print_title "Dropping the Cluster"

    #
    # 
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    if [ "$*" ]; then
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
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddNode
    runFunctionalTest testStopStartNode

    runFunctionalTest testConfig
    runFunctionalTest testConfigFail

    runFunctionalTest testCreateAccount01
    runFunctionalTest testCreateAccount02
    runFunctionalTest testCreateDatabase

    runFunctionalTest testCreateBackup
    runFunctionalTest testRestoreBackup
    runFunctionalTest testRemoveBackup
    
    runFunctionalTest testRunScript
    runFunctionalTest testRollingRestart

    runFunctionalTest testDrop
fi

endTests


