#! /bin/bash
MYNAME=$(basename "$0")
MYBASENAME=$(basename "$0" .sh)
MYDIR=$(dirname "$0")
VERBOSE=""
VERSION="0.0.3"

LOG_OPTION="--wait"
DEBUG_OPTION=""

CONTAINER_SERVER=""
CONTAINER_IP=""
CLUSTER_NAME=""
LAST_CONTAINER_NAME=""
OPTION_VENDOR="mariadb"
PROVIDER_VERSION="10.2"
OPTION_NUMBER_OF_NODES="1"

N_CONTAINERS=0
N_CLUSTERS=0
MY_CONTAINERS=""

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh
source ./include_lxc.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]

 $MYNAME - Test script for s9s to check Galera on LXC.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the server and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o registerServer   Creates a new cmon-cloud container server. 
  o createCluster    Creates a cluster on VMs created on the fly.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,number-of-nodes:,vendor:,leave-nodes,enable-ssl" \
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

        *)
            printError "Unknown option '$1'."
            exit 6
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
# This is where we create a cluster. This test is called multiple times, so
# there will be more than one of these clusters.
#
function createCluster()
{
    local node_name
    local cluster_name
    local nodes
    local n
    local master_cluster_id_option

    print_title "Creating a Cluster"


    #
    # The cluster name.
    #
    let N_CLUSTERS+=1
    cluster_name=$(printf "cluster_%03d" $N_CLUSTERS)
    
    if [ "$N_CLUSTERS" -gt 1 ]; then
        master_cluster_id_option="--remote-cluster-id=1"
    fi

    #
    # Composing a list of container names.
    #
    n=0
    while [ "$n" -lt "$OPTION_NUMBER_OF_NODES" ]; do
        let N_CONTAINERS+=1
        node_name=$(printf "%s_%02d_%06d" "${MYBASENAME}" "$N_CONTAINERS" "$$")

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi

        MY_CONTAINERS+=" $node_name"
        nodes+="$node_name"
        let n+=1
    done

    #
    # Creating a Cluster.
    #
    mys9s cluster \
        --create \
        --template="ubuntu" \
        --cluster-name="$cluster_name" \
        --cluster-type=galera \
        --provider-version="$PROVIDER_VERSION" \
        --vendor=$OPTION_VENDOR \
        --nodes="$nodes" \
        --containers="$nodes" \
        $master_cluster_id_option \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    while true; do 
        CLUSTER_ID=$(find_cluster_id $cluster_name)
        
        if [ "$CLUSTER_ID" != 'NOT-FOUND' ]; then
            break;
        fi

        echo "Cluster '$cluster_name' not found."
        s9s cluster --list --long
        sleep 5
    done

    if [ "$CLUSTER_ID" -gt 0 2>/dev/null ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    check_container_ids --galera-nodes

    wait_for_cluster_started "$cluster_name"
    mys9s cluster --list --long

    if [ -n "$master_cluster_id_option" ]; then
        mys9s replication --list
    fi
}

function testEnableBinaryLogging()
{
    local node_name

    print_title "Enabling Binary Logging"

    if [ "$N_CLUSTERS" -gt 0 ]; then
        node_name=$(galera_node_name --cluster-id 1)
        if [ -n "$node_name" ]; then
            success "  o Will enable binary logging on $node_name, ok."
        else
            failure "Could not find node in cluster 1."
        fi

        mys9s nodes \
            --enable-binary-logging \
            --nodes=$node_name \
            --cluster-id=1 \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
    else
        failure "No cluster was created?"
        return 1
    fi
        
    mys9s replication --list
}

function testFailover()
{
    local master_node
    local slave_node

    print_title "Testing failover"
    master_node=$(galera_node_name --cluster-id 1)
    slave_node=$(galera_node_name --cluster-id 2)

    if [ -n "$master_node" ]; then
        success "  o Master is $master_node, ok."
    else
        failure "Could not find master."
        return 1
    fi

    if [ -n "$slave_node" ]; then 
        success "  o Slave node is $slave_node, ok."
    else
        failure "Could not find slave."
        return 1
    fi

    mys9s replication \
        --failover \
        --master=$master_node:3306 \
        --slave=$slave_node:3306 \
        --cluster-id=2 \
        --remote-cluster-id=1 \
        $LOG_OPTION \
        $DEBUG_OPTION
        
    check_exit_code $?
    mys9s replication --list
    mysleep 60
    mys9s replication --list
}

function testBack()
{
    local master_node
    local slave_node

    print_title "Testing Switch Back After Failover"
    master_node=$(galera_node_name --cluster-id 2)
    slave_node=$(galera_node_name --cluster-id 1)

    if [ -n "$master_node" ]; then
        success "  o Master is $master_node, ok."
    else
        failure "Could not find master."
        return 1
    fi

    if [ -n "$slave_node" ]; then 
        success "  o Slave node is $slave_node, ok."
    else
        failure "Could not find slave."
        return 1
    fi

    mys9s replication \
        --failover \
        --master=$master_node:3306 \
        --slave=$slave_node:3306 \
        --cluster-id=1 \
        --remote-cluster-id=2 \
        $LOG_OPTION \
        $DEBUG_OPTION
        
    check_exit_code $?
    mys9s replication --list
    mysleep 60
    mys9s replication --list

    mys9s node --list --long
}

function testDeleteContainers()
{
    local container_name

    print_title "Deleting Containers"

    for container_name in $MY_CONTAINERS; do
        mys9s container --delete $container_name --wait
        check_exit_code $?
    done
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
        runFunctionalTest testEnableBinaryLogging
        runFunctionalTest createCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testEnableBinaryLogging
    runFunctionalTest createCluster
    runFunctionalTest testFailover
    runFunctionalTest testBack
    runFunctionalTest testDeleteContainers
fi

endTests
