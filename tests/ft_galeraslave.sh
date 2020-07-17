#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check Galera clusters.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=STRING The SQL server provider version.
  --leave-nodes    Do not destroy the nodes at exit.

SUPPORTED TESTS:
  o testPing             Pings the controller.
  o testCreateCluster    Creates a Galera cluster.

EXAMPLE
 ./$MYNAME --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,vendor:,leave-nodes" \
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
            ;;

        --log)
            shift
            LOG_OPTION="--log"
            ;;

        --server)
            shift
            CONTAINER_SERVER="$1"
            shift
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

        --leave-nodes)
            shift
            OPTION_LEAVE_NODES="true"
            ;;

        --)
            shift
            break
            ;;
    esac
done

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local container_name
    local nNodes=2
    local n
    
    print_title "Creating a Galera Cluster"
    begin_verbatim

    for ((n=0;n<nNodes;++n)); do
        echo "Creating node #$n"

        container_name="$(printf "ft_galeraslave_%08d_node%02d" "$$" "$n")"
        nodeName=$(create_node --autodestroy $container_name)

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi
          
        nodes+="$nodeName"

        if [ -z "$FIRST_ADDED_NODE" ]; then
            FIRST_ADDED_NODE="$nodeName"
        fi
    
        LAST_ADDED_NODE="$nodeName"
    done
    
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "  o Cluster ID is $CLUSTER_ID, OK"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
    mys9s node --list --long
    mys9s cluster --stat
    end_verbatim
}

function testDemoteNode()
{
    local retCode=""

    print_title "Demoting Node $LAST_ADDED_NODE"
    begin_verbatim
        
    mys9s cluster \
        --demote-node \
        --cluster-id=1 \
        --nodes=$LAST_ADDED_NODE \
        --log
    
    retCode=$?
    if [ "$retCode" -ne 0 ]; then
        warning "Return code is $retCode."
    fi
    #check_exit_code $?

    mys9s node --stat
    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddSlave()
{
    local container_name

    print_title "Adding a New Node as Slave"
    cat <<EOF
This test will add a new node as slave to the cluster created in the previous
test as a single node postgresql cluster.
EOF

    container_name="$(printf "ft_galeraslave_%08d_slave%02d" "$$" "$n")"
    nodeName=$(create_node --autodestroy $container_name)

    mys9s node --list --long

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$nodeName?slave" \
        $LOG_OPTION \
        $DEBUG_OPTION
   
    check_exit_code $? 

    mys9s node --list --long
    mys9s node --stat    
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddSlave
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    #runFunctionalTest testDemoteNode
    runFunctionalTest testAddSlave
fi

endTests

