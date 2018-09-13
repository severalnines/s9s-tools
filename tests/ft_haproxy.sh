#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"
LOG_OPTION="--wait"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
OPTION_INSTALL=""
HAPROXY_IP=""

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME9="${MYBASENAME}_19_$$"


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
  
  $MYNAME - Test script for s9s to check various error conditions.

  -h, --help          Print this help and exit.
  --verbose           Print more messages.
  --print-json        Print the JSON messages sent and received.
  --log               Print the logs while waiting for the job to be ended.
  --print-commands    Do not print unit test info, print the executed commands.
  --install           Just install the cluster and haproxy, then exit.
  --reset-config      Remove and re-generate the ~/.s9s directory.
  --server=SERVER     Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,install,reset-config,\
server:" \
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

    #
    # Creating a container.
    #
    mys9s server \
        --register \
        --servers="lxc://$container_server" 

    check_exit_code_no_job $?

    #mys9s server --list --long
    #check_exit_code_no_job $?

    check_container_server \
        --server-name "$CONTAINER_SERVER" \
        --class       "CmonLxcServer"
}

function createCluster()
{
    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1" \
        --containers="$CONTAINER_NAME1" \
        $LOG_OPTION 

    check_exit_code $?
}

#
# This test will add a HaProxy node.
#
function testAddHaProxy()
{
    print_title "Adding a HaProxy Node"
    
    #
    # Adding haproxy to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=1 \
        --nodes="haProxy://$CONTAINER_NAME9" \
        --containers="$CONTAINER_NAME9" \
        $LOG_OPTION
    
    check_exit_code $?

    #
    #
    #
    print_title "Checking HaProxy State"
    HAPROXY_IP=$(haproxy_node_name)

    wait_for_node_state "$HAPROXY_IP" "CmonHostOnline"
    if [ $? -ne 0 ]; then
        failure "HaProxy $HAPROXY_IP is not in CmonHostOnline state"
        mys9s node --list --long
        mys9s node --stat $HAPROXY_IP
    else
        mys9s node --stat $HAPROXY_IP
    fi
}

function testStopHaProxy()
{
    #
    #
    #
    print_title "Stopping Node"

    mys9s container --stop --wait "$CONTAINER_NAME9"
    check_exit_code $?

    #
    # Checking that the HaProxy goes into offline state.
    #
    print_title "Waiting HapProxy to go Off-line"
    wait_for_node_state "$HAPROXY_IP" "CmonHostOffLine"

    if [ $? -ne 0 ]; then
        failure "HaProxy $HAPROXY_IP is not in CmonHostOffLine state"
        mys9s node --list --long
        mys9s node --stat $HAPROXY_IP
    else
        mys9s node --stat $HAPROXY_IP
    fi
}

function testStartHaProxy()
{
    #
    #
    #
    print_title "Starting Node"

    mys9s container --start --wait "$CONTAINER_NAME9"
    check_exit_code $?

    #
    # Checking that the HaProxy goes into offline state.
    #
    print_title "Waiting HapProxy to go On-line"
    wait_for_node_state "$HAPROXY_IP" "CmonHostOnline"

    if [ $? -ne 0 ]; then
        failure "HaProxy $HAPROXY_IP is not in CmonHostOnLine state"
        mys9s node --list --long
        mys9s node --stat $HAPROXY_IP
    else
        mys9s node --stat $HAPROXY_IP
    fi
}

function destroyContainers()
{
    #
    #
    #
    print_title "Destroying Containers"

    mys9s container --delete --wait "$CONTAINER_NAME1"
    mys9s container --delete --wait "$CONTAINER_NAME9"
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
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testAddHaProxy
    runFunctionalTest testStopHaProxy
    runFunctionalTest testStartHaProxy
    runFunctionalTest destroyContainers
fi

endTests
