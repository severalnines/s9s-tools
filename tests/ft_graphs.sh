#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
MYHOSTNAME="$(hostname)"
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

CONTAINER_SERVER="$MYHOSTNAME"
CONTAINER_IP=""

CLUSTER_NAME="${MYBASENAME}_$$"
LAST_CONTAINER_NAME=""

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]
 Test script for s9s to check various error conditions.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the server and the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o registerServer   Registers a new container server. No software installed.
  o createCluster    Creates a new cluster on containers on the fly.
  o testGraphs       Creates and checks graphs.
  o deleteContainer  Deletes the previously created container.

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
}

function createCluster()
{
    local node001="ft_containers_lxc_11_$$"
    local node_ip
    local container_id

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster"
    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --nodes="$node001" \
        --containers="$node001" \
        $LOG_OPTION

    check_exit_code $?

    while true; do 
        CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
        
        if [ "$CLUSTER_ID" != 'NOT-FOUND' ]; then
            break;
        fi

        echo "Cluster '$CLUSTER_NAME' not found."
        s9s cluster --list --long
        sleep 5
    done

    if [ "$CLUSTER_ID" -gt 0 2>/dev/null ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    check_container_ids --galera-nodes

    return 0
}

function testGraphs()
{
    local graphs
    local graph

    print_title "Waiting for a while"
    echo "So that we have some data collected..."
    sleep 300

    print_title "Testing Graphs"
    graphs+="cpuuser diskfree diskreadspeed diskreadwritespeed diskwritespeed "
    graphs+="diskutilization memfree memutil neterrors netreceivedspeed "
    graphs+="netreceiveerrors nettransmiterrors netsentspeed netspeed "
    graphs+="sqlcommands sqlcommits sqlconnections sqlopentables sqlqueries "
    graphs+="sqlreplicationlag sqlslowqueries swapfree"

    for graph in $graphs; do
        mys9s node \
               --stat \
               --cluster-id=1 \
               --graph=$graph    
        
        mys9s node \
               --stat \
               --cluster-id=1 \
               --density \
               --graph=$graph 

        check_exit_code_no_job $?
        sleep 10
    done
}

#
# This will delete the containers we created before.
#
function deleteContainer()
{
    local containers
    local container

    containers="ft_containers_lxc_11_$$"

    print_title "Deleting Containers"

    #
    # Deleting all the containers we created.
    #
    for container in $containers; do
        mys9s container \
            --cmon-user=system \
            --password=secret \
            --delete \
            $LOG_OPTION \
            "$container"
    
        check_exit_code $?
    done

    #mys9s job --list
}


#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest registerServer
    runFunctionalTest checkServer
    runFunctionalTest createCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testGraphs
    runFunctionalTest deleteContainer
fi

endTests
