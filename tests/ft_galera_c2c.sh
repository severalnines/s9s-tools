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
PROVIDER_VERSION="10.3"
OPTION_NUMBER_OF_NODES="1"

N_CONTAINERS=0
N_CLUSTERS=0
MY_CONTAINERS=""

node_ip11=""
node_ip12=""
node_ip13=""

node_ip23=""

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

  --vendor=STRING  Use the given Galera vendor.
  --provider-version=VERSION The SQL server provider version.

SUPPORTED TESTS:
  o registerServer      Creates a new cmon-cloud container server. 
  o createMasterCluster Creates a cluster to be user as a master cluster.
  o testBinaryLog       Enabled the binary logging on all the servers.
  o testBinaryLog       
  o testFailover        Testing failover between clusters.
  o testBack            Testing failover between clusters.
  o testFinalize        Deleting containers.
  
EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,number-of-nodes:,vendor:,leave-nodes" \
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
#
#
function createMasterCluster()
{
    local node_name1
    local node_name2
    local node_name3
    local cluster_name="master_cluster"
    local nodes
    local n
    local master_cluster_id_option

    print_title "Creating the Master Cluster"

    #
    # Composing a list of container names.
    #
    node_name1=$(printf "%s_11_%06d" "${MYBASENAME}" "$$")
    node_ip11=$(create_node --autodestroy --template="ubuntu" $node_name1)

    node_name2=$(printf "%s_12_%06d" "${MYBASENAME}" "$$")
    node_ip12=$(create_node --autodestroy --template="ubuntu" $node_name2)

    node_name3=$(printf "%s_13_%06d" "${MYBASENAME}" "$$")
    node_ip13=$(create_node --autodestroy --template="ubuntu" $node_name3)

    nodes="$node_ip11;$node_ip12;$node_ip13"

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

    wait_for_cluster_started "$cluster_name"
    mys9s cluster --list --long

    if [ -n "$master_cluster_id_option" ]; then
        mys9s replication --list
    fi
}

#
#
#
function createSlaveCluster()
{
    local node_name1
    local cluster_name="slave_cluster"
    local nodes
    local master_cluster_id_option

    print_title "Creating the Slave Cluster"

    #
    # Composing a list of container names.
    #
    node_name1=$(printf "%s_21_%06d" "${MYBASENAME}" "$$")
    node_ip21=$(create_node --autodestroy --template="ubuntu" $node_name1)
    
    nodes="$node_ip21"
    
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
        --remote-cluster-id=1 \
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

    wait_for_cluster_started "$cluster_name"

    mys9s cluster --list --long
    if [ -n "$master_cluster_id_option" ]; then
        mys9s replication --list
    fi
}

function testBinaryLog()
{
    local node_name
    local node_list

    print_title "Enabling Binary Logging"

    node_list=$(s9s node \
        --list --cluster-id=1 --long --batch | \
        grep ^g | \
        awk '{ print $5}')

    for node_name in $node_list; do
        mys9s nodes \
            --enable-binary-logging \
            --nodes=$node_name \
            --cluster-id=1 \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
    done
        
    mys9s replication --list
}

function testFailover()
{
    print_title "Testing Failover"

    mys9s node --list --long

    mys9s replication \
        --failover \
        --master=$node_ip12:3306 \
        --slave=$slave_ip21:3306 \
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
    print_title "Testing Switch Back After Failover"
    
    mys9s node --list --long

    mys9s replication \
        --failover \
        --master=$node_ip11:3306 \
        --slave=$node_ip21:3306 \
        --cluster-id=2 \
        --remote-cluster-id=1 \
        $LOG_OPTION \
        $DEBUG_OPTION
        
    check_exit_code $?
    mys9s replication --list
    mysleep 60
    mys9s replication --list

    mys9s node --list --long
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
        runFunctionalTest createMasterCluster
        runFunctionalTest testBinaryLog
        runFunctionalTest createSlaveCluster
        runFunctionalTest testFailover
        runFunctionalTest testBack
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createMasterCluster
    runFunctionalTest testBinaryLog
    runFunctionalTest createSlaveCluster
    runFunctionalTest testFailover
    runFunctionalTest testBack
fi

endTests
