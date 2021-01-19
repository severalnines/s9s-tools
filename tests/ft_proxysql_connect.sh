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

N_DATABASE_NODE=1
PROXY_SERVER=""

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
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
 
  $MYNAME - Checks if a created ProxySql server can be connected.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --provider-version=STRING The SQL server provider version.
  --leave-nodes    Do not destroy the nodes at exit.

SUPPORTED TESTS:
  o testPing           Pinging the controller.
  o testCreateCluster  Creates a cluster.
  o testCreateDatabase Creates some accounts and databases (pipas:password).
  o testAddProxySql    Adds a ProxySQL node.
  o testConnect02      Connects ProxySql with pipas.
  o testUploadData     Uploads data through the ProxySQL server as proxydemo.

EXAMPLE
 ./$MYNAME --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,leave-nodes" \
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

function testCreateReplicationCluster()
{
    local nodes
    local nodeName
    local exitCode

    #
    #
    #
    print_title "Creating MySql Replication Cluster."
    begin_verbatim

    nodeName=$(create_node --autodestroy "${MYBASENAME}_00_$$")
    nodes+="$nodeName?master;"
    FIRST_ADDED_NODE=$nodeName
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_01_$$")
    SECOND_ADDED_NODE=$nodeName
    nodes+="$nodeName?slave;"
 
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

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME" 

    mys9s replication --list --long

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "REPLICATION" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"

    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$SECOND_ADDED_NODE" \
        --state          "Online"
    
    end_verbatim
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateGaleraCluster()
{
    local nodes
    local node
    local nodeName
    local n_nodes_added=0

    #
    # Creating a galera cluster.
    #
    print_title "Creating a Cluster"

    while true; do
        echo "Creating node #$n_nodes_added"

        node=$(printf "ft_proxysql_connect_node%02d_$$" "$n_nodes_added")
        nodeName=$(create_node --autodestroy "$node")
        
        if [ "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$nodeName"

        if [ -z "$FIRST_ADDED_NODE" ]; then
            FIRST_ADDED_NODE="$nodeName"
        fi

        #
        #
        #
        let n_nodes_added+=1
        if [ "$n_nodes_added" -ge "$N_DATABASE_NODE" ]; then
            break;
        fi
    done
    
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
}

function testCreateCluster()
{
    testCreateReplicationCluster
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local userName="pipas"
    local databaseName="test_database"
    local userPassword="password"

    print_title "Creating Database"
    cat <<EOF | paragraph
EOF

    begin_verbatim

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="$databaseName" \
        --batch
    
    check_exit_code_no_job $?
    
    #
    # Creating accounts. They have various allow host settings and access
    # rights.
    # 
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="$userName:$userPassword@%" \
        --privileges="*.*:ALL" \
        --batch
    
    check_exit_code_no_job $?
    
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="test_user1:password@192.%" \
        --privileges="$databaseName.*:ALL" \
        --batch
    
    check_exit_code_no_job $?
    
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="test_user2:password@10.10.1.23" \
        --privileges="$databaseName.*:ALL" \
        --batch
    
    check_exit_code_no_job $?
    end_verbatim
}


#
# This test will add a proxy sql node.
#
function testAddProxySql()
{
    local node
    local nodes
    local nodeName

    #
    #
    #
    print_title "Adding a ProxySQL Node"
    begin_verbatim

    nodeName=$(create_node --autodestroy "ft_proxysql_connect_proxy00_$$")
    PROXY_SERVER="$nodeName"
    nodes+="proxySql://$nodeName"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        --log --debug
    
    check_exit_code $?
    end_verbatim
}

function testAddProxySql1()
{
    local node
    local nodes
    local nodeName

    #
    #
    #
    print_title "Adding a ProxySQL Node"
    begin_verbatim

    nodeName=$(create_node --autodestroy "ft_proxysql_connect_proxy01_$$")
    PROXY_SERVER="$nodeName"
    nodes+="proxySql://$nodeName"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        --log --debug
    
    check_exit_code $?
    end_verbatim
}

function testConnect02()
{
    local sql_host="$PROXY_SERVER"
    local sql_port="6033"
    local db_name="test_database"
    local sql_user="pipas"
    local sql_password="password"
    local reply

    print_title "Testing Connection ${sql_user}@${sql_host}."
    begin_verbatim

    #
    # Executing a simple SQL statement using the account we created.
    #
cat <<EOF
        mysql \
            --disable-auto-rehash \
            --batch \
            -h$sql_host \
            -P$sql_port \
            -u$sql_user \
            -p$sql_password \
            $db_name \
            -e "SELECT 41+1"
EOF

    reply=$(\
            mysql \
                --disable-auto-rehash \
                --batch \
                -h$sql_host \
                -P$sql_port \
                -u$sql_user \
                -p$sql_password \
                $db_name \
                -e "SELECT 41+1" \
        | \
            tail -n +2 \
        )
    
    if [ "$reply" != "42" ]; then
        failure "Failed SQL statement on ${sql_user}@${sql_host}: '$reply'."
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
    if [ -n "$1" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateCluster
        runFunctionalTest testCreateDatabase
        runFunctionalTest testAddProxySql
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateDatabase
    runFunctionalTest testAddProxySql
    runFunctionalTest testConnect02
fi

endTests


