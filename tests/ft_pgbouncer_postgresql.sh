#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--wait"
DEBUG_OPTION=""

CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
OPTION_INSTALL=""
PGBOUNCER_IP=""
PGBOUNCER_IPS=""
PROVIDER_VERSION="11"

CONTAINER_NAME1="${MYBASENAME}_node_1_$$"
CONTAINER_NAME2="${MYBASENAME}_node_2_$$"
CONTAINER_NAME3="${MYBASENAME}_node_3_$$"

cd $MYDIR
source ./include.sh
source ./include_lxc.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
  
  $MYNAME - Test script for s9s to check various error conditions.

  -h, --help          Print this help and exit.
  --verbose           Print more messages.
  --print-json        Print the JSON messages sent and received.
  --log               Print the logs while waiting for the job to be ended.
  --debug             Print more messages.
  --print-commands    Do not print unit test info, print the executed commands.
  --install           Just install the cluster and pgbouncer, then exit.
  --reset-config      Remove and re-generate the ~/.s9s directory.
  --cluster-name=name Use the given cluster name to run tests.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-json,log,debug,print-commands,\
install,reset-config,cluster-name:" \
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
            OPTION_VERBOSE="--verbose"
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

        --debug)
            shift
            DEBUG_OPTION="--debug"
            ;;

        --print-json)
            shift
            OPTION_PRINT_JSON="--print-json"
            ;;

        --print-commands)
            shift
            DEBUG_OPTION="--debug --print-request"
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

        --cluster-name)
            CLUSTER_NAME="$2"
	        shift 2
            ;;

        --)
            shift
            break
            ;;

        *) 
            printError "Unhandled option $1."
            break
            ;;
    esac
done

function createCluster()
{
    local retCode

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    cat <<EOF | paragraph
  Here we create a cluster on which the PgBouncer will be tested. The cluster
  will be created on the LXC server usng LXC containers.
EOF

    begin_verbatim

    NODES="$CONTAINER_NAME1;PgBouncer://$CONTAINER_NAME1"
    CONTAINERS="$CONTAINER_NAME1?template=ubuntu"
    
    # Creating the cluster.
    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=postgresql \
        --provider-version=$PROVIDER_VERSION \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --nodes="$NODES" \
        --containers="$CONTAINERS" \
        $LOG_OPTION \
        $DEBUG_OPTION

    retCode=$?
    check_exit_code $retCode
    if [ $retCode -ne 0 ]; then
        end_verbatim
        return 1
    fi

    PGBOUNCER_IP=$(pgbouncer_node_name)
    wait_for_node_state "$PGBOUNCER_IP" "CmonHostOnline"
    mys9s node --list --long
    end_verbatim
}

function accountTest()
{
    print_title "Testing the creation of an account"

    begin_verbatim

    # Creating the database
    mys9s cluster \
        --create-database \
        --cluster-name="$CLUSTER_NAME" \
        --db-name="testdatabase" \
    
    check_exit_code $?

    # Creating an account.
    mys9s account \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --account="${PROJECT_OWNER}:${PROJECT_OWNER}" \
        --privileges="testdatabase.*:ALL"

    node_name=$(postgresql_node_name --cluster-id 1)
    check_postgresql_account \
        --hostname          "$node_name" \
        --port              "5432" \
        --account-name      "${PROJECT_OWNER}" \
        --account-password  "${PROJECT_OWNER}" \
        --database-name     "testdatabase" \
        --create-table      \
        --insert-into \
        --select \
        --drop-table


    end_verbatim
}

function checkPgBouncer()
{
    local pgbouncer_ip

    #
    #
    #
    print_title "Checking PgBouncer State"
    begin_verbatim

    PGBOUNCER_IPS=$(pgbouncer_node_name)

    for pgbouncer_ip in $PGBOUNCER_IPS; do
        wait_for_node_state "$pgbouncer_ip" "CmonHostOnline"
        if [ $? -ne 0 ]; then
            failure "PgBouncer $pgbouncer_ip is not in CmonHostOnline state"
            mys9s node --list --long
            mys9s node --stat $pgbouncer_ip
        else
            success "  o PgBouncer $pgbouncer_ip is in CmonHostOnline state."
            mys9s node --stat $pgbouncer_ip
        fi
    done

    mys9s node --list --long
    end_verbatim
}

function testPgBouncerConnect()
{
    print_title "Testing the PgBouncer Server"

    begin_verbatim
    mys9s node --list --long

    if [ -n "$PGBOUNCER_IPS" ]; then
        success "  o PgBouncer IP found, ok."
    else
        failure "No PgBouncer address."
    fi

    for PGBOUNCER_IP in $PGBOUNCER_IPS; do    
        check_postgresql_account \
            --hostname          "$PGBOUNCER_IP" \
            --port              "6432" \
            --account-name      "${PROJECT_OWNER}" \
            --account-password  "${PROJECT_OWNER}" \
            --database-name     "testdatabase" \
            --create-table      \
            --insert-into \
            --select \
            --drop-table
    done

    end_verbatim
}

function unregisterPgBouncer()
{
    local line
    local retcode

    print_title "Unregistering PgBouncer Node"
    cat <<EOF | paragraph
  This test will unregister the PgBouncer node. 
EOF

    begin_verbatim

    #
    # Unregister by the owner should be possible.
    #
    mys9s node \
        --unregister \
        --nodes="PgBouncer://$PGBOUNCER_IP:6432"

    check_exit_code_no_job $?

    mys9s node --list --long
    line=$(s9s node --list --long --batch | grep '^b')
    if [ -z "$line" ]; then 
        success "  o The PgBouncer node is no longer part of he cluster, ok."
    else
        failure "The PgBouncer is still there after unregistering the node."
    fi

    end_verbatim
}

function registerPgBouncer()
{
    local line
    local retcode

    print_title "Registering PgBouncer Node"
    cat <<EOF | paragraph
  This test will register the PgBouncer node that was previously unregistered.
EOF

    begin_verbatim
   
    #
    # Registering the pgbouncer host here.
    #
    mys9s node \
        --register \
        --cluster-id=1 \
        --nodes="PgBouncer://$PGBOUNCER_IP" \
        --log 

    check_exit_code $?
       
    line=$(s9s node --list --long --batch | grep '^b')
    if [ -n "$line" ]; then 
        success "  o The PgBouncer node is part of he cluster, ok."
    else
        failure "The PgBouncer is not part of the cluster."
    fi

    wait_for_node_state "$PGBOUNCER_IP" "CmonHostOnline"
    mys9s node --list --long
    end_verbatim
}


function destroyContainers()
{
    #
    #
    #
    print_title "Destroying Containers"
    begin_verbatim

    mys9s container --delete \
        "$CONTAINER_NAME1" $LOG_OPTION $DEBUG_OPTION

    check_exit_code $?

    end_verbatim
}

#
# Running the requested tests.
#
runFunctionalTest startTests
runFunctionalTest reset_config
runFunctionalTest grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest accountTest
    runFunctionalTest checkPgBouncer
    runFunctionalTest testPgBouncerConnect
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest accountTest
    runFunctionalTest checkPgBouncer
    runFunctionalTest testPgBouncerConnect
    runFunctionalTest unregisterPgBouncer
    runFunctionalTest registerPgBouncer
    runFunctionalTest destroyContainers
fi

endTests
