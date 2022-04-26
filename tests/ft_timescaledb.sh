#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.3"

LOG_OPTION="--wait"
DEBUG_OPTION="--debug"

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""
PROVIDER_VERSION="12"

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
    cat <<EOF
  This test will create a new PostgreSQL cluster with the TimescaleDb extension
  enabled.
EOF

    begin_verbatim

    #
    # Creating containers.
    #
    nodeName=$(create_node --autodestroy $node1)
    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName

    #
    # Creating a PostgreSQL cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --with-timescaledb \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    end_verbatim

    #
    #
    #
    print_title "Waiting until the Cluster Started"
    begin_verbatim
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
        --owner      "${PROJECT_OWNER}" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    
    check_node \
        --node       "$FIRST_ADDED_NODE" \
        --ip-address "$FIRST_ADDED_NODE" \
        --port       "8089" \
        --config     "/etc/postgresql/$PROVIDER_VERSION/main/postgresql.conf" \
        --owner      "${PROJECT_OWNER}" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "${PROJECT_OWNER}" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "POSTGRESQL_SINGLE" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"

    end_verbatim
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

    begin_verbatim

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
        --owner      "${PROJECT_OWNER}" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint

    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    print_title "Dropping the Cluster"
    begin_verbatim

    #
    # 
    #
    mys9s cluster \
        --drop \
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
    runFunctionalTest testDrop
fi

endTests


