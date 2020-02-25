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
PROXYSQL_IP=""
OPTION_INSTALL=""

OPTION_VENDOR="10gen"
PROVIDER_VERSION="3.2"

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME3="${MYBASENAME}_13_$$"
CONTAINER_NAME9="${MYBASENAME}_19_$$"

export S9S_DEBUG_PRINT_REQUEST="true"

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check the ProxySQL support. 

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given MongoDb vendor.
  --provider-version=STRING The database server provider version.
  --server=SERVER  Use the given server to create containers.
  --install        Just install the cluster and exit.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:,\
install,provider-version:,vendor:" \
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

        --vendor)
            shift
            OPTION_VENDOR="$1"
            shift
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
    local class

    print_title "Registering Container Server"
    begin_verbatim

    #
    # Creating a container.
    #
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?

    mys9s server --list --long
    check_exit_code_no_job $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$CONTAINER_SERVER" \
        | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonLxcServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
        exit 1
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state
    end_verbatim
}

#
# This is where we create a MongoDb cluster.
#
function createCluster()
{
    #
    # Creating a Cluster.
    #
    print_title "Creating a MongoDB Cluster on LXC"
    cat <<EOF | paragraph
  Here we create a mongodb cluster. For some reason the mongodb create_cluster
  job is very different from other create_cluster jobs, there is no "nodes" for
  some reason. 
EOF

    begin_verbatim

    mys9s cluster \
        --create \
        --template="ubuntu" \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=mongodb \
        --provider-version="$PROVIDER_VERSION" \
        --vendor="$OPTION_VENDOR" \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1;$CONTAINER_NAME2" \
        --containers="$CONTAINER_NAME1;$CONTAINER_NAME2" \
        $LOG_OPTION 

    check_exit_code $?

    mys9s cluster --stat

    #
    # Checking if the cluster is started.
    #
    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
    
    mys9s node --list --long
    mys9s node --stat 
    #mys9s node --list --print-json
    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local nodes

    print_title "Adding a node"
    begin_verbatim

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$CONTAINER_NAME3" \
        --containers="$CONTAINER_NAME3" \
        $LOG_OPTION
    
    check_exit_code $?
    end_verbatim
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    print_title "Performing Rolling Restart"
    begin_verbatim

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    check_exit_code $?
    
    wait_for_cluster_started "$CLUSTER_NAME"
    mys9s cluster --stat

    mys9s node --list --long
    mys9s node --stat 
    #mys9s node --list --print-json
    end_verbatim
}

#
# Destroying all the this test created.
#
function destroyContainers()
{
    print_title "Destroying Containers"
    begin_verbatim

    mys9s container --delete --wait "$CONTAINER_NAME1"
    mys9s container --delete --wait "$CONTAINER_NAME2"
    #mys9s container --delete --wait "$CONTAINER_NAME3"
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
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testRollingRestart
    #runFunctionalTest testAddNode
    runFunctionalTest destroyContainers
fi

endTests
