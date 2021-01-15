#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--wait"
DEBUG_OPTION="--debug"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
OPTION_INSTALL=""
HAPROXY_IP=""
HAPROXY_IPS=""

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME3="${MYBASENAME}_13_$$"
CONTAINER_NAME9="${MYBASENAME}_19_$$"

OPTION_RO_PORT=""
OPTION_RW_PORT=""
PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

#export S9S_DEBUG_PRINT_REQUEST="true"

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
  --install           Just install the cluster and haproxy, then exit.
  --reset-config      Remove and re-generate the ~/.s9s directory.
  --server=SERVER     Use the given server to create containers.

  --vendor=STRING  Use the given Galera vendor.
  --provider-version=VERSION The SQL server provider version.
  --ro-port=INTEGER   The read-only port for the haproxy server.
  --rw-port=INTEGER   The read-write port for the haproxy server.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,debug,print-commands,install,\
reset-config,server:,vendor:,provider-version:,ro-port:,rw-port:" \
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

        --ro-port)
            OPTION_RO_PORT="$2"
            shift 2
            ;;

        --rw-port)
            OPTION_RW_PORT="$2"
            shift 2
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
        --vendor=$OPTION_VENDOR \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1" \
        --containers="$CONTAINER_NAME1" \
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
        --account="pipas:pipas" \
        --privileges="*.*:ALL"

    node_name=$(galera_node_name --cluster-id 1)
    check_mysql_account \
        --hostname          "$node_name" \
        --port              "3306" \
        --account-name      "pipas" \
        --account-password  "pipas" \
        --database-name     "testdatabase" \
        --create-table      \
        --insert-into \
        --select \
        --drop-table


    end_verbatim
}

#
# This test will add a HaProxy node.
#
function testAddHaProxy()
{
    local nodes_option

    print_title "Adding a HaProxy Node"
    cat <<EOF | paragraph
  In this test we add a HaProxy node to the previously created cluster. The
  HaProxy will be installed on a container that is created by the same job.
EOF

    begin_verbatim

    #
    # Adding haproxy to the cluster.
    #
    nodes_option="haProxy://$CONTAINER_NAME9"
    if [ -n "$OPTION_RO_PORT" -a -n "$OPTION_RW_PORT" ]; then
        nodes_option+="?rw_port=$OPTION_RW_PORT&ro_port=$OPTION_RO_PORT"
    fi

    mys9s cluster \
        --add-node \
        --cluster-id=1 \
        --nodes="$nodes_option" \
        --containers="$CONTAINER_NAME9" \
        $LOG_OPTION $DEBUG_OPTION
    
    check_exit_code $?
    mys9s node --list --long

    end_verbatim

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
    local rw_port

    print_title "Testing the HaProxy Server"

    begin_verbatim
    mys9s node --list --long

    if [ -n "$HAPROXY_IPS" ]; then
        success "  o HaProxy IPs found, ok."
    else
        failure "No HaProxy address."
    fi

    rw_port="3307"
    if [ -n "$OPTION_RW_PORT" ]; then
        rw_port="$OPTION_RW_PORT" 
    fi

    for HAPROXY_IP in $HAPROXY_IPS; do
        check_mysql_account \
            --hostname          "$HAPROXY_IP" \
            --port              "$rw_port" \
            --account-name      "pipas" \
            --account-password  "pipas" \
            --database-name     "testdatabase" \
            --create-table      \
            --insert-into \
            --select \
            --drop-table
    done

    end_verbatim
}

function testStopHaProxy()
{
    print_title "Stopping HaProxy Node"
    cat <<EOF | paragraph
  In this test we stop the HapRoxy node and check the the node status is changed
  on the Cmon Controller. 
EOF

    begin_verbatim

    mys9s container --stop "$CONTAINER_NAME9" $LOG_OPTION $DEBUG_OPTION
    check_exit_code $?
    end_verbatim

    #
    # Checking that the HaProxy goes into offline state.
    #
    print_title "Waiting HapProxy to go Off-line"
    begin_verbatim

    wait_for_node_state "$HAPROXY_IP" "CmonHostOffLine"

    if [ $? -ne 0 ]; then
        failure "HaProxy $HAPROXY_IP is not in CmonHostOffLine state"
        mys9s node --list --long
        mys9s node --stat $HAPROXY_IP
    else
        success "  o HaProxy $HAPROXY_IP is in CmonHostOffLine state."
        mys9s node --stat $HAPROXY_IP
    fi

    end_verbatim
}

function testStartHaProxy()
{
    #
    #
    #
    print_title "Starting Node"
    begin_verbatim

    mys9s container --start "$CONTAINER_NAME9" $LOG_OPTION $DEBUG_OPTION
    check_exit_code $?
    end_verbatim

    #
    # Checking that the HaProxy goes into offline state.
    #
    print_title "Waiting HapProxy to go On-line"
    begin_verbatim
    wait_for_node_state "$HAPROXY_IP" "CmonHostOnline"

    if [ $? -ne 0 ]; then
        failure "HaProxy $HAPROXY_IP is not in CmonHostOnLine state."
        mys9s node --list --long
        mys9s node --stat $HAPROXY_IP
    else
        success "  o HaProxy $HAPROXY_IP is in CmonHostOnLine state, ok."
        mys9s node --stat $HAPROXY_IP
    fi
    end_verbatim
}

function unregisterHaProxy()
{
    local line
    local retcode

    print_title "Unregistering HaProxy Node"
    cat <<EOF | paragraph
  This test will unregister the HaProxy node. 
EOF

    begin_verbatim

    #
    # Unregister by the owner should be possible.
    #
    mys9s node \
        --unregister \
        --nodes="haproxy://$HAPROXY_IP:9600"

    check_exit_code_no_job $?

    mys9s node --list --long
    line=$(s9s node --list --long --batch | grep '^y')
    if [ -z "$line" ]; then 
        success "  o The HaProxy node is no longer part of he cluster, ok."
    else
        failure "The HaProxy is still there after unregistering the node."
    fi

    end_verbatim
}

function registerHaProxy()
{
    local line
    local retcode

    print_title "Registering HaProxy Node"
    cat <<EOF | paragraph
  This test will register the HaProxy node that was previously unregistered.
EOF

    begin_verbatim
   
    #
    # Registering the maxscale host here.
    #
    mys9s node \
        --register \
        --cluster-id=1 \
        --nodes="haproxy://$HAPROXY_IP" \
        --log 

    check_exit_code $?
    
    mys9s node --list --long
    line=$(s9s node --list --long --batch | grep '^h')
    if [ -n "$line" ]; then 
        success "  o The HaProxy node is part of he cluster, ok."
    else
        warning "The HaProxy is not part of the cluster."
        
        mysleep 15
        line=$(s9s node --list --long --batch | grep '^h')
        if [ -n "$line" ]; then 
            success "  o The HaProxy node is part of he cluster, ok."
        else
            failure "The HaProxy is not part of the cluster."
        fi
    fi

    wait_for_node_state "$HAPROXY_IP" "CmonHostOnline"
    end_verbatim
}

function destroyContainers()
{
    #
    #
    #
    print_title "Destroying Containers"
    begin_verbatim

    mys9s container --delete "$CONTAINER_NAME1" $LOG_OPTION $DEBUG_OPTION
    check_exit_code $?

    mys9s container --delete "$CONTAINER_NAME9" $LOG_OPTION $DEBUG_OPTION
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
    runFunctionalTest testAddHaProxy
    runFunctionalTest testHaProxyConnect
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testAddHaProxy
    runFunctionalTest testHaProxyConnect
    runFunctionalTest testStopHaProxy
    runFunctionalTest testStartHaProxy
    runFunctionalTest unregisterHaProxy
    runFunctionalTest registerHaProxy
    runFunctionalTest destroyContainers
fi

endTests
