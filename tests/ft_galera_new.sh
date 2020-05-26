#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"

LOG_OPTION="--wait"
DEBUG_OPTION=""

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

export SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"

cd $MYDIR
source ./include.sh

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

SUPPORTED TESTS
  o testCreateCluster       Creates a cluster.
  o testCreateClusterFail1  Fails to create a cluster by re-using nodes.
  o testCreateClusterDupl1  Creates cluster with duplicate name.
  o testRemoveClusterFail   Fails to remove cluster 0.

EXAMPLE
 ./$MYNAME --print-commands --server=core1 --reset-config --install

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
            DEBUG_OPTION="--debug"
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
    local node1="ft_galera_new_001"
    local node2="ft_galera_new_002"
    local nodes
    local nodeName
    local exitCode
    local command_line
    #
    # Creating a Galera cluster.
    #
    print_title "Creating a Galera Cluster by Re-using the Nodes"
    cat <<EOF
  In this test we first create a cluster, then immediately we drop the cluster
  and so make the nodes available for creating a new cluster on them. After
  these we create a new cluster re-using the nodes and so drive the controller
  into a situation when it needs to uninstall the software the first cluster
  creation installed.

EOF
    begin_verbatim

    echo "Creating node #0"
    nodeName=$(create_node --autodestroy $node1)
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
    echo "Creating node #1"
    nodeName=$(create_node --autodestroy $node2)
    nodes+="$nodeName;"
    LAST_ADDED_NODE="$nodeName"
 
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "  o Cluster ID is $CLUSTER_ID, ok"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"

    #
    # Dropping the cluster we just created.
    #
    print_subtitle "Dropping the Galera Cluster"
    mys9s cluster --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    # Need to stop the daemon...
    command_line="sudo killall -KILL mysql mysqld_safe mysqld"

    echo "$FIRST_ADDED_NODE@$USER# $command_line"
    $SSH $FIRST_ADDED_NODE -- "$command_line"

    echo "$LAST_ADDED_NODE@$USER# $command_line"
    $SSH $LAST_ADDED_NODE -- "$command_line"

    #
    # Creating the a new cluster using the same name, the same nodes.
    #
    print_subtitle "Creating a Cluster Using the Same Nodes"
   

    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "  o Cluster ID is $CLUSTER_ID, ok"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
    end_verbatim
}

function testCreateClusterFail1()
{
    local exitCode

    #
    #
    #
    print_title "Creating Cluster with Failure"
    cat <<EOF
  This test will try to create a MySQL replication cluster by re-using a node
  that is part of an already existing cluster. This should fail, no way to 
  co-locate MySQL servers on the same computer.

EOF
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-type=mysqlreplication \
        --nodes="$LAST_ADDED_NODE" \
        --vendor="percona" \
        --provider-version="5.6" \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    if [ $exitCode -eq 0 ]; then
        failure "Re-using node in a new cluster should have failed."
    else
        success "  o Cluster that re-using a node failed, ok."
    fi
    end_verbatim
}

function testCreateClusterDupl1()
{
    local node1="ft_galera_new_011"
    local newClusterName="${CLUSTER_NAME}~1"
    local newClusterId
    local nodes
    local nodeName
    local exitCode

    print_title "Creating a Galera Cluster with Same Name"
    cat <<EOF
  This test will try to create a new cluster with the name that is already used
  by a previously created cluster. In this case the cluster should be created,
  but renamed to CLUSTERNAME~1 on the fly.

EOF
    begin_verbatim

    echo "Creating node #1"
    nodeName=$(create_node --autodestroy $node1)
    nodes+="$nodeName;"
 
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
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?

    mys9s cluster --list --long
    mys9s node --list --long

    newClusterId=$(find_cluster_id $newClusterName)
    if [ "$newClusterId" -gt 0 ]; then
        success "  o Cluster ID is $newClusterId, ok"
    else
        failure "Cluster ID '$newClusterId' is invalid"
    fi
    
    wait_for_cluster_started "$newClusterName"
    end_verbatim
}

function testCreateClusterDupl2()
{
    local node1="ft_galera_new_021"
    local newClusterName="${CLUSTER_NAME}~2"
    local newClusterId
    local nodes
    local nodeName
    local exitCode

    print_title "Creating a Galera Cluster with Same Name"
    cat <<EOF
  Yet another cluster with the same name. This should be renamed to
  CLUSTERNAME~2 of course.
EOF
    begin_verbatim

    echo "Creating node #2"
    nodeName=$(create_node --autodestroy $node1)
    nodes+="$nodeName;"
 
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
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?

    mys9s cluster --list --long
    mys9s node --list --long

    newClusterId=$(find_cluster_id $newClusterName)
    if [ "$newClusterId" -gt 0 ]; then
        success "  o Cluster ID is $newClusterId, ok"
    else
        failure "Cluster ID '$newClusterId' is invalid"
    fi
    
    wait_for_cluster_started "$newClusterName"

    end_verbatim
}

function testRemoveClusterFail()
{
    local exitCode 

    print_title "Removing Cluster 0"
    cat <<EOF
  This test will try to remove cluster 0 and checks that this actually fails.
EOF
    begin_verbatim

    mys9s cluster \
        --drop \
        --cluster-id=0 \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    if [ $exitCode -eq 0 ]; then
        failure "Removing the cluster with ID 0 should have failed."
    else
        success "  o Removing cluster 0 failed, ok."
    fi

    end_verbatim
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testCreateCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateClusterFail1
    runFunctionalTest testCreateClusterDupl1
    runFunctionalTest testCreateClusterDupl2
    runFunctionalTest testRemoveClusterFail
fi

endTests

