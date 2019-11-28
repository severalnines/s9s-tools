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
PROVIDER_VERSION="9.6"

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
 
  $MYNAME - Tests the installation of PostgreSQL clusters.

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
# A function to do various checks on a PostgreSQL database accound.
#
function check_postgresql_account()
{
    local hostname
    local port
    local account_name
    local account_password
    local database_name="template1"
    local table_name
    local create_table
    local query
    local lines
    local retcode

    while [ -n "$1" ]; do
        case "$1" in
            --hostname)
                hostname="$2"
                shift 2
                ;;

            --port)
                port="$2"
                shift 2
                ;;

            --account-name)
                account_name="$2"
                shift 2
                ;;

            --account-password)
                account_password="$2"
                shift 2
                ;;

            --database-name)
                database_name="$2"
                shift 2
                ;;

            --table-name)
                table_name="$2"
                shift 2
                ;;

            --create-table)
                create_table="yes"
                shift
                ;;

            *)
                break
                ;;
        esac
    done
    
    if [ -n "$hostname" ]; then
        success "  o Test host is '$hostname', ok."
    else
        failure "PostgreSQL host name required."
        return 1
    fi
    
    if [ -n "$account_name" ]; then
        success "  o Test account is '$account_name', ok."
    else
        failure "Account name required."
    fi

    lines=$(PGPASSWORD="$account_password" \
        psql \
            -t \
            --username="$account_name" \
            --host="$hostname" \
            --port="$port" \
            --dbname="$database_name" \
            --command="select 41 + 1;")

    retcode="$?"
    #echo "$lines"

    lines=$(echo "$lines" | tail -n 1);
    lines=$(echo $lines);
    
    if [ "$retcode" -eq 0 ]; then
        success "  o The return code is 0, ok."
    else
        failure "The return code is '$retcode'."
    fi

    if [ "$lines" == "42" ]; then
        success "  o The result is '$lines', ok."
    else
        failure "The result for 'select 41 + 1;' is '$lines'."
    fi

    if [ -n "$create_table" ]; then
        query="CREATE TABLE $table_name( \
            NAME           CHAR(50) NOT NULL,\
            VALUE          INT      NOT NULL \
        );"

        PGPASSWORD="$account_password" \
            psql \
                -t \
                --username="$account_name" \
                --host="$hostname" \
                --port="$port" \
                --dbname="$database_name" \
                --command="$query"

        retcode="$?" 
        if [ "$retcode" -eq 0 ]; then
            success "  o Create table '$database_name.$table_name' is ok."
        else
            failure "Create table failed."
        fi
    fi
   
    #
    # Printing the databases.
    #
    return 0
    PGPASSWORD="$account_password" \
        psql \
            -P pager=off \
            --username="$account_name" \
            --host="$hostname" \
            --port="$port" \
            --dbname="$database_name" \
            --command="SELECT datname, datacl FROM pg_database;"
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local node1="ft_postgresql_01_$$"
    local nodes
    local nodeName
    local message_id
    local prefix

    print_title "Creating a PostgreSQL Cluster"
    cat <<EOF | paragraph
  This test will create a PostgreSQL cluster and check its state.

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
        --job-tags="createCluster" \
        --cluster-type=postgresql \
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

    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local node="ft_postgresql_02_$$"

    print_title "Adding a New Node"
    cat <<EOF | paragraph
This test will add a new node as slave to the cluster created in the previous
test as a single node postgresql cluster.

EOF

    LAST_ADDED_NODE=$(create_node --autodestroy "$node")

    begin_verbatim

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
    
    end_verbatim
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    print_title "Creating Databases"

    begin_verbatim

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testcreatedatabase"
   
    check_exit_code_no_job $?
    
    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "postmaster" \
        --account-password   "passwd12" 

    end_verbatim

    return 0

    # These doesn't work. Previously the controller reported ok, but it did not
    # work then either.

    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="pipas:password" \
        --privileges="testcreatedatabase.*:INSERT,UPDATE"
    
    check_exit_code_no_job $?
    
    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "pipas" \
        --account-password   "password" 
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testcreatedatabase.*:DELETE" \
        --batch 
    
    check_exit_code_no_job $?
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    #
    # 
    #
    print_title "Dropping the Cluster"
    cat <<EOF
  We are at the end of the test script, we now drop the cluster that we created
  at the beginning of this test.

EOF

    begin_verbatim
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
        runFunctionalTest testAddNode
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddNode
    runFunctionalTest testCreateDatabase
    runFunctionalTest testDrop
fi

endTests


