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

cd $MYDIR
source ./include.sh
source ./include_user.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s create multiple clusters on containers.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the server and the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o createUserSisko  Creates a user to work with.
  o registerServer   Registers a new container server. No software installed.
  o createCluster1   Creates a cluster.

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
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state
    end_verbatim
}

function createCluster1()
{
    local container1="node_001"
    local container2="node_002"
    local container3="node_003"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="my_cluster_1" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$container1;$container2;$container3" \
        --containers="$container1;$container2;$container3" \
        $LOG_OPTION
    end_verbatim
}

function createCluster2()
{
    local container1="node_004"
    local container2="node_005"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="my_cluster_2" \
        --cluster-type=galera \
        --provider-version="5.7" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$container1;$container2" \
        --containers="$container1;$container2" \
        $LOG_OPTION
    end_verbatim
}

function createCluster3()
{
    local container1="node_006"
    local container2="node_007"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="my_cluster_3" \
        --cluster-type=galera \
        --provider-version="10.2" \
        --vendor=mariadb \
        --cloud=lxc \
        --nodes="$container1;$container2" \
        --containers="$container1;$container2" \
        $LOG_OPTION
    end_verbatim
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

#s9s event --list --with-event-job &
#EVENT_HANDLER_PID=$!

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest createUserSisko
    runFunctionalTest registerServer
    runFunctionalTest createCluster1
    runFunctionalTest createCluster2
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createUserSisko
    runFunctionalTest registerServer
    runFunctionalTest createCluster1
    runFunctionalTest createCluster2
    runFunctionalTest createCluster3
fi

#kill $EVENT_HANDLER_PID
endTests
