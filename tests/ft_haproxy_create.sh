#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--log"
DEBUG_OPTION="--debug"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
OPTION_INSTALL=""
HAPROXY_IP=""
HAPROXY_IPS=""

CONTAINER_NAME1="${MYBASENAME}_node_1_$$"
CONTAINER_NAME2="${MYBASENAME}_node_2_$$"
CONTAINER_NAME3="${MYBASENAME}_node_3_$$"

CONTAINER_NAME_HAPROXY_1="${MYBASENAME}_haproxy_1_$$"
CONTAINER_NAME_HAPROXY_2="${MYBASENAME}_haproxy_2_$$"

cd $MYDIR
source include.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

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
  --install           Just install the cluster and haproxy, then exit.
  --reset-config      Remove and re-generate the ~/.s9s directory.
  --server=SERVER     Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,debug,print-commands,install,\
reset-config,server:" \
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

        --)
            shift
            break
            ;;
    esac
done

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

if [ -z "$CONTAINER_SERVER" ]; then
    printError "No container server specified."
    printError "Use the --server command line option to set the server."
    exit 6
fi

#
# This will register the container server. 
#
function registerServer()
{
    local container_server="$CONTAINER_SERVER"
    local old_ifs="$IFS"
    local n_names_found
    local class
    local file

    print_title "Registering Container Server"
    begin_verbatim

    #
    # Creating a container.
    #
    mys9s server \
        --register \
        --servers="lxc://$container_server" 

    check_exit_code_no_job $?
    end_verbatim
}

function createCluster()
{
    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    cat <<EOF | paragraph
  Here we create a cluster on which the HaProxy will be tested. The cluster will
  be created on the LXC server usng LXC containers.
EOF

    begin_verbatim

    # Creating the cluster.
    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="$PROVIDER_VERSION" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1;haProxy://$CONTAINER_NAME_HAPROXY_1" \
        --containers="$CONTAINER_NAME1;$CONTAINER_NAME_HAPROXY_1" \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

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
        --account="$PROJECT_OWNER:$PROJECT_OWNER" \
        --privileges="*.*:ALL"

    node_name=$(galera_node_name --cluster-id 1)
    check_mysql_account \
        --hostname          "$node_name" \
        --port              "3306" \
        --account-name      "$PROJECT_OWNER" \
        --account-password  "$PROJECT_OWNER" \
        --database-name     "testdatabase" \
        --create-table      \
        --insert-into \
        --select \
        --drop-table


    end_verbatim
}

function checkHaProxy()
{
    #
    #
    #
    print_title "Checking HaProxy State"
    begin_verbatim

    HAPROXY_IPS=$(haproxy_node_name)

    for HAPROXY_IP in $HAPROXY_IPS; do
        wait_for_node_state "$HAPROXY_IP" "CmonHostOnline"
        if [ $? -ne 0 ]; then
            failure "HaProxy $HAPROXY_IP is not in CmonHostOnline state"
            mys9s node --list --long
            mys9s node --stat $HAPROXY_IP
        else
            success "  o HaProxy $HAPROXY_IP is in CmonHostOnline state."
            mys9s node --stat $HAPROXY_IP
        fi
    done

    mys9s node --list --long
    end_verbatim
}

function testHaProxyConnect()
{
    print_title "Testing the HaProxy Server"

    begin_verbatim
    if [ -n "$HAPROXY_IPS" ]; then
        success "  o HaProxy IP found, ok."
    else
        failure "No HaProxy address."
    fi

    for HAPROXY_IP in $HAPROXY_IPS; do    
        check_mysql_account \
            --hostname          "$HAPROXY_IP" \
            --port              "3307" \
            --account-name      "$PROJECT_OWNER" \
            --account-password  "$PROJECT_OWNER" \
            --database-name     "testdatabase" \
            --create-table      \
            --insert-into \
            --select \
            --drop-table
    done

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

    mys9s container --delete \
        "$CONTAINER_NAME_HAPROXY_1" \
        $LOG_OPTION $DEBUG_OPTION
    
    check_exit_code $?
    
#    mys9s container --delete \
#        "$CONTAINER_NAME_HAPROXY_2" \
#        $LOG_OPTION $DEBUG_OPTION

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
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest checkHaProxy
    runFunctionalTest testHaProxyConnect
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest checkHaProxy
    runFunctionalTest testHaProxyConnect
    runFunctionalTest destroyContainers
fi

endTests
