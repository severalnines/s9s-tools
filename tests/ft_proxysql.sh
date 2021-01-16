#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--wait"
DEBUG_OPTION=""

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
PROXYSQL_IP=""
OPTION_INSTALL=""

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME9="${MYBASENAME}_19_$$"

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
 
  $MYNAME - Test script for s9s to check the ProxySQL support. 

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --install        Just install the cluster and exit.
  --proxy2         Install ProxySQL 2.0.

SUPPORTED TESTS:
  o registerServer     Registers a container server for containers.
  o createCluster      Creates a cluster to test.
  o testAddProxySql    Adds a ProxySQL server to the cluster.
  o testStopProxySql   Stops the added ProxySQL server.
  o testStartProxySql  Starts the ProxySQL server that was stopped.
  o destroyContainers  Destroys the previously created containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:,\
install,proxy2" \
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

        --print-json)
            shift
            OPTION_PRINT_JSON="--print-json"
            ;;

        --print-commands)
            shift
            DONT_PRINT_TEST_MESSAGES="true"
            PRINT_COMMANDS="true"
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

        --install)
            shift
            OPTION_INSTALL="--install"
            ;;

        --proxy2)
            shift
            OPTION_POXYSQL_VERSION="--provider-version=2"
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

function createCluster()
{
    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1" \
        --containers="$CONTAINER_NAME1" \
        --template="ubuntu" \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    end_verbatim
}

#
# This test will add a ProxySql node.
#
function testAddProxySql()
{
    print_title "Adding a ProxySql Node"
    begin_verbatim

    #
    # Adding ProxySql to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="proxySql://$CONTAINER_NAME9" \
        --template="ubuntu" \
        --containers="$CONTAINER_NAME9" \
        $LOG_OPTION \
        $DEBUG_OPTION \
        $OPTION_POXYSQL_VERSION \
    
    check_exit_code $?
    end_verbatim

    #
    #
    #
    print_subtitle "Checking ProxySql State"
    begin_verbatim

    PROXYSQL_IP=$(proxysql_node_name)

    wait_for_node_state "$PROXYSQL_IP" "CmonHostOnline"
    if [ $? -ne 0 ]; then
        failure "ProxySql $PROXYSQL_IP is not in CmonHostOnline state"
        mys9s node --list --long
        mys9s node --stat $PROXYSQL_IP
    else
        mys9s node --stat $PROXYSQL_IP
    fi

    end_verbatim
}

function unregisterProxySql()
{
    local line
    local retcode

    print_title "Unregistering ProxySql Node"
    cat <<EOF | paragraph
  This test will unregister the ProxySql node. 
EOF

    begin_verbatim

    #
    # Unregister by the owner should be possible.
    #
    mys9s node \
        --unregister \
        --nodes="proxysql://$PROXYSQL_IP:6032"

    check_exit_code_no_job $?

    mys9s node --list --long
    line=$(s9s node --list --long --batch | grep '^y')
    if [ -z "$line" ]; then 
        success "  o The ProxySql node is no longer part of he cluster, ok."
    else
        failure "The ProxySql is still there after unregistering the node."
    fi

    end_verbatim
}

function registerProxySql()
{
    local line
    local retcode

    print_title "Registering ProxySql Node"
    cat <<EOF | paragraph
  This test will register the ProxySql node that was previously unregistered.
EOF

    begin_verbatim
   
    #
    # Registering the maxscale host here.
    #
    mys9s node \
        --register \
        --cluster-id=1 \
        --nodes="proxysql://$PROXYSQL_IP" \
        --log 

    check_exit_code $?
    
    mys9s node --list --long
    line=$(s9s node --list --long --batch | grep '^y')
    if [ -n "$line" ]; then 
        success "  o The ProxySql node is part of he cluster, ok."
    else
        warning "The ProxySql is not part of the cluster."
        
        mysleep 15
        line=$(s9s node --list --long --batch | grep '^y')
        if [ -n "$line" ]; then 
            success "  o The ProxySql node is part of he cluster, ok."
        else
            failure "The ProxySql is not part of the cluster."
        fi
    fi

    wait_for_node_state "$PROXYSQL_IP" "CmonHostOnline"
    end_verbatim
}


function testStopProxySql()
{
    print_title "Stopping ProxySQL Node"
    begin_verbatim

    mys9s container --stop --wait "$CONTAINER_NAME9"
    check_exit_code $?

    #
    # Checking that the ProxySql goes into offline state.
    #
    print_title "Waiting ProxySql to go Off-line"
    wait_for_node_state "$PROXYSQL_IP" "CmonHostOffLine"

    if [ $? -ne 0 ]; then
        failure "ProxySql $PROXYSQL_IP is not in CmonHostOffLine state"
        mys9s node --list --long
        mys9s node --stat $PROXYSQL_IP
    else
        success "  o ProxySql $PROXYSQL_IP is in CmonHostOffLine state, OK."
        mys9s node --stat $PROXYSQL_IP
    fi

    end_verbatim
}

function testStartProxySql()
{
    #
    #
    #
    print_title "Starting ProxySQL Node"
    begin_verbatim

    mys9s container --start --wait "$CONTAINER_NAME9"
    check_exit_code $?

    #
    # Checking that the ProxySql goes into online state.
    #
    wait_for_node_state "$PROXYSQL_IP" "CmonHostOnline"

    if [ $? -ne 0 ]; then
        failure "ProxySql $PROXYSQL_IP is not in CmonHostOnLine state."
        mys9s node --list --long
        mys9s node --stat $PROXYSQL_IP
    else
        success "  o ProxySql $PROXYSQL_IP is in CmonHostOnLine state, OK."
        mys9s node --stat $PROXYSQL_IP
    fi

    end_verbatim
}

function destroyContainers()
{
    print_title "Destroying Containers"
    begin_verbatim
    mys9s container --delete --wait "$CONTAINER_NAME1"
    mys9s container --delete --wait "$CONTAINER_NAME9"
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
        runFunctionalTest registerServer
        runFunctionalTest createCluster
        runFunctionalTest testAddProxySql
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testAddProxySql
    runFunctionalTest unregisterProxySql
    runFunctionalTest registerProxySql
    runFunctionalTest testStopProxySql
    runFunctionalTest testStartProxySql
    runFunctionalTest destroyContainers
fi

endTests
