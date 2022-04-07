#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME="ftgalera" #$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"

LOG_OPTION="--wait"
DEBUG_OPTION=""
LOG_OPTION="--log"
DEBUG_OPTION="--debug"

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""


PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

OPTION_INSTALL=""
OPTION_NUMBER_OF_NODES="3"
OPTION_VENDOR="percona"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

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
  --leave-nodes    Do not destroy the nodes at exit.
  --enable-ssl     Enable the SSL once the cluster is created.
  
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=VERSION The SQL server provider version.

  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:
  o testPing              Pings the controller.
  o testCreateCluster     Creates a Galera cluster.
  o testSetupAudit        Sets up audit logging.
  o testSetConfig01       Changes some configuration values for the cluster.
  o testSetConfig02       More configuration checks.
  o testRestartNode       Restarts one node of the cluster.
  o testStopStartNode     Stops, then starts a node.
  o testCreateAccount     Creates an account on the cluster.
  o testCreateDatabase    Creates a database on the cluster.
  o testUploadData        If test data is found uploads data to the cluster.
  o testAddNode           Adds a new database node.
  o testAddRemoveProxySql Adds a ProxySql node to the cluster.
  o testAddRemoveHaProxy  Adds, then removes a HaProxy node.
  o testRemoveNode        Removes a data node from the cluster.
  o testRollingRestart    Executes a rolling restart on the cluster.
  o testStop              Stops the cluster.
  o testStart             Starts the cluster.

EXAMPLE
 ./$MYNAME --print-commands --server=core1 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,number-of-nodes:,vendor:,leave-nodes,enable-ssl,\
os-vendor:,os-release:" \
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

        --leave-nodes)
            shift
            OPTION_LEAVE_NODES="true"
            ;;

        --enable-ssl)
            shift
            OPTION_ENABLE_SSL="true"
            ;;

        --os-vendor)
            OPTION_OS_VENDOR="$2"
            shift 2
            ;;

        --os-release)
            OPTION_OS_RELEASE="$2"
            shift 2
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
    local node_ip
    local exitCode
    local node_serial=1
    local node_name
    local exitCode

    print_title "Creating a Galera Cluster"

    begin_verbatim
    while [ "$node_serial" -le "$OPTION_NUMBER_OF_NODES" ]; do
        node_name=$(printf "${MYBASENAME}node%03d$$" "$node_serial")

        echo "Creating node #$node_serial"
        node_ip=$(create_node \
            --os-vendor   "$OPTION_OS_VENDOR"  \
            --os-release  "$OPTION_OS_RELEASE" \
            --autodestroy "$node_name")
    
        exitCode=$?
        check_exit_code $exitCode
        if [ "$exitCode" -ne 0 ]; then
            end_verbatim
            return 1
        fi

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$node_ip"

        if [ -z "$FIRST_ADDED_NODE" ]; then
            FIRST_ADDED_NODE="$node_ip"
        fi

        let node_serial+=1
    done
     
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --job-tags="createCluster" \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        --force \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    check_exit_code $exitCode
    if [ "$exitCode" -ne 0 ]; then
        end_verbatim
        return 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME" 
    if [ $? -eq 0 ]; then
        success "  o The cluster got into STARTED state and stayed there, ok. "
    else
        failure "Failed to get into STARTED state."
        mys9s cluster --stat
        mys9s job --list 
    fi
    
    end_verbatim

    #
    # Checking the controller, the nodes and the cluster.
    #
    print_subtitle "Checking the State of the Cluster&Nodes"

    begin_verbatim
    mys9s cluster --stat

    check_controller \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    
    for node in $(echo "$nodes" | tr ';' ' '); do
        check_node \
            --node             "$node" \
            --ip-address       "$node" \
            --port             "3306" \
            --config-basename  "my.cnf" \
            --owner            "pipas" \
            --group            "testgroup" \
            --cdt-path         "/$CLUSTER_NAME" \
            --status           "CmonHostOnline" \
            --no-maint
    done

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "GALERA" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"

    end_verbatim

    #
    # One more thing: if the option is given we enable the SSL here, so we test
    # everything with this feature.
    #
    if [ -n "$OPTION_ENABLE_SSL" ]; then
        print_title "Enabling SSL"
        cat <<EOF
  This test will enable SSL on a cluster, then the cluster will be stopped and 
  started. Then the test will check if the cluster is indeed started.
EOF
        begin_verbatim
        mys9s cluster --enable-ssl --cluster-id=$CLUSTER_ID \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
        
        mys9s cluster --stop --cluster-id=$CLUSTER_ID \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
        
        mys9s cluster --start --cluster-id=$CLUSTER_ID \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
    
        wait_for_cluster_started "$CLUSTER_NAME" 
        end_verbatim
    fi
}

function testSetupAudit()
{
    print_title "Setting up Audit Logging"
    if [ "$OPTION_VENDOR" == "codership" ]; then
        cat <<EOF | paragraph
  Audit logging is not supported with codership, this test is now skipped.
EOF
        return 0
    fi

    begin_verbatim
    mys9s cluster \
        --setup-audit-logging \
        --cluster-id=1 \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    end_verbatim
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

    print_title "Checking the configuration"
    
    begin_verbatim

    #
    # Listing the configuration values. The exit code should be 0.
    #
    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE 

    check_exit_code_no_job $?

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
    
    check_exit_code_no_job $?
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    check_exit_code_no_job $?

    if [ "$value" != "$newValue" ]; then
        failure "Configuration value should be $newValue not $value"
    else
        success "  o Configuration value is $value, ok."
    fi

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        $name

    end_verbatim
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

    print_title "Changing the configuration"
    begin_verbatim
    #
    # Changing a configuration value.
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=$name \
        --opt-group=MYSQLD \
        --opt-value=$newValue
   
    check_exit_code_no_job $?
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    check_exit_code_no_job $?

    if [ "$value" != "$newValue" ]; then
        failure "Configuration value should be $newValue not $value"
    else
        success "  o Configuration value is $value, ok."
    fi

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        'max*'

    end_verbatim
}

#
# This test will call a --restart on the node.
#
function testRestartNode()
{
    local exitCode

    print_title "Restarting Node"
   
    begin_verbatim

    #
    # Restarting a node. 
    #
    mys9s node \
        --restart \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?

    end_verbatim
}

#
# This test will first call a --stop then a --start on a node. Pretty basic
# stuff.
#
function testStopStartNode()
{
    local exitCode
    print_title "Stopping and starting node"

    begin_verbatim
    #
    # First stop.
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION \
        $DEBUG_OPTION
   
    check_exit_code $?
    
    #
    # Then start.
    #
    mys9s node \
        --start \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?

    end_verbatim
}

#
# Creating a new account on the cluster.
#
function testCreateAccount()
{
    local userName

    print_title "Testing account creation."
    begin_verbatim

    #
    # This command will create a new account on the cluster.
    #
    if [ -z "$CLUSTER_ID" ]; then
        failure "No cluster ID found."
    else
        success "  o Cluster ID is $CLUSTER_ID, ok."
    fi

    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="john_doe:password@1.2.3.4" \
        --with-database
    
    check_exit_code_no_job $?

    mys9s account --list --cluster-id=$CLUSTER_ID 
    mys9s account --list --cluster-id=$CLUSTER_ID john_doe

    #
    # Checking if the account is created.
    #
    userName=$(s9s account --list --cluster-id=$CLUSTER_ID john_doe)
    if [ "$userName" != "john_doe" ]; then
        failure "Account is not in the account list ($userName)."
        mys9s account --list --long --cluster-id=$CLUSTER_ID
        mys9s account --list --cluster-id=$CLUSTER_ID john_doe
    else
        success "  o Account found, ok."
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
    else
        success "  o Accound disappeared, ok."
    fi

    end_verbatim
}


#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local userName

    print_title "Creating Database"
    begin_verbatim

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testCreateDatabase" \
        --batch

    check_exit_code_no_job $?
    
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
    else
        success "  o User $userName was created, ok."
    fi

    #
    # This command will grant some rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testCreateDatabase.*:DELETE,DROP" 
   
    check_exit_code_no_job $?

    mys9s account --list --long
    end_verbatim
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
    begin_verbatim

    #
    # Creating a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --db-name=$db_name

    check_exit_code_no_job $?

    #
    # Creating a new account on the cluster.
    #
    mys9s account \
        --create \
        --account="$user_name:$password" \
        --privileges="$db_name.*:ALL"
    
    check_exit_code_no_job $?

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
    else
        success "  o The classic 'SELECT 41+1' returned 42, ok."
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
        else
            success "  o Uploaded data, ok."
        fi

        let count+=1
        if [ "$count" -gt 99 ]; then
            break
        fi
    done

    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local node_name="${MYBASENAME}addednode$$"
    local nodes

    print_title "Adding a node"
    begin_verbatim

    LAST_ADDED_NODE=$(create_node \
        --os-vendor   "$OPTION_OS_VENDOR"  \
        --os-release  "$OPTION_OS_RELEASE" \
        --autodestroy "$node_name")
    nodes+="$LAST_ADDED_NODE"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
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
    
    print_title "Removing Node"
    begin_verbatim

    #
    # Removing the last added node. We do this by cluster name for that is the
    # more complicated one.
    #
    mys9s cluster \
        --remove-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="$LAST_ADDED_NODE" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    end_verbatim
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local ret_code
    print_title "Testing Rolling Restart"
    cat <<EOF
  This test will try to execute a rollingrestart job on the cluster. If the
  number of nodes is less than 3 this should fail, if it is at least 3 it should
  be successful. Either way the cluster should remain operational which is
  checked in consequite tests.

EOF
    
    begin_verbatim

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    ret_code=$?
    if [ $OPTION_NUMBER_OF_NODES -lt 3 ]; then
        if [ $ret_code -ne 0 ]; then
            success "  o The cluster is too small for rollingrestart, ok."
        else
            failure "The cluster is too small, this should have failed."
        fi
    else
        check_exit_code $ret_code
    fi
    
    wait_for_cluster_started "$CLUSTER_NAME" 
    end_verbatim
}

#
# Stopping the cluster.
#
function testStop()
{
    print_title "Stopping Cluster"
    begin_verbatim

    #
    # Stopping the cluster.
    #
    mys9s cluster \
        --stop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    end_verbatim
}

#
# Starting the cluster.
#
function testStart()
{
    print_title "Starting Cluster"
    begin_verbatim

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --start \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION
    
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
    if [ -n "$1" ]; then
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
    runFunctionalTest testPing

    runFunctionalTest testCreateCluster
    runFunctionalTest testSetupAudit

    runFunctionalTest testSetConfig01
    runFunctionalTest testSetConfig02

    runFunctionalTest testRestartNode
    runFunctionalTest testStopStartNode

    #runFunctionalTest testCreateAccount
    #runFunctionalTest testCreateDatabase

    #runFunctionalTest testAddNode

    #runFunctionalTest testAddRemoveProxySql
    #runFunctionalTest testAddRemoveHaProxy
    #runFunctionalTest testAddRemoveMaxScale

    #runFunctionalTest testRemoveNode
    #runFunctionalTest testRollingRestart
    #runFunctionalTest testStop
    #runFunctionalTest testStart
fi

endTests


