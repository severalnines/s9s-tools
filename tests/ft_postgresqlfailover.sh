#! /bin/bash
MYNAME=$(basename "$0")
MYBASENAME=$(basename "$MYNAME" .sh)
MYDIR=$(dirname "$0")
VERSION="0.0.1"
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
OPTION_RESET_CONFIG=""
CONTAINER_SERVER=""
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""
N_CONTAINERS=0

cd $MYDIR
source include.sh

PROVIDER_VERSION=$POSTGRESQL_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - PostgreSQL failover script.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config" \
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

        --)
            shift
            break
            ;;
    esac
done

#
# Prints the state of the node, e.g. CmonHostOnline.
#
function wait_for_node_become_slave()
{
    local nodeName="$1"
    local state
    local waited=0
    local stayed=0
    local max_wait=240

    while true; do
        state=$(s9s node --list --batch --long --node-format="%R" "$nodeName")
        if [ "$state" == "slave" ]; then
            let stayed+=1
        else
            let stayed=0
        fi

        if [ "$stayed" -gt 10 ]; then
            return 0
        fi

        if [ "$waited" -gt $max_wait ]; then
            return 1
        fi

        let waited+=1
        sleep 1
    done

    return 2
}

#
# Waits until the node becomes stable in CmonHostOnline state.
#
function wait_for_node_online()
{
    local nodeName="$1"
    local state
    local waited=0
    local stayed=0
    local max_wait=240

    while true; do
        state=$(s9s node --list --batch --long --node-format="%S" "$nodeName")
        if [ "$state" == "CmonHostOnline" ]; then
            let stayed+=1
        else
            let stayed=0
        fi

        if [ "$stayed" -gt 10 ]; then
            return 0
        fi

        if [ "$waited" -gt $max_wait ]; then
            return 1
        fi

        let waited+=1
        sleep 1
    done

    return 2
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode
    local container_name


    print_title "Creating a PostgreSQL Cluster"

    begin_verbatim
    let N_CONTAINERS+=1
    container_name=$(printf "${MYBASENAME}_%02d_$$" "$N_CONTAINERS")
    nodeName=$(create_node --autodestroy "$container_name")

    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName
    
    #
    # Creating a PostgreSQL cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local container_name
    
    print_title "Adding a Node"

    begin_verbatim
    let N_CONTAINERS+=1
    container_name=$(printf "${MYBASENAME}_%02d_$$" "$N_CONTAINERS")    
    LAST_ADDED_NODE=$(create_node --autodestroy "$container_name")

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE?slave" \
        $LOG_OPTION
   
    check_exit_code $?

    mys9s node --stat --cluster-id=$CLUSTER_ID
    end_verbatim
}

#
# This will stop the master node so we can observe what happens with the
# failover.
#
function testStopMaster()
{
    local timeLoop="0"
    local jobId

    #
    # Tring to stop the first added node. 
    # This should fail, because the master is protected.
    #
    print_title "Stopping node on $FIRST_ADDED_NODE"
    #mys9s node --list --long 

    begin_verbatim
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION

    if [ $? -eq 0 ]; then
        failure "The controller should have protected master agains restart."
        return 1
    else
        success "  o The controller protected the master, ok."
    fi
    
    end_verbatim

    #
    # Stopping it manually.
    #
    print_title "Stopping postgresql Directly on $FIRST_ADDED_NODE"
    cat <<EOF | paragraph
  In this test we execute the pg_ctlcluster command to stop the postgresql on a
  node and then check that the failover job is executed and finished.
EOF

    begin_verbatim

    echo "pg_lsclusters"
    $SSH "$FIRST_ADDED_NODE" sudo pg_lsclusters

    echo "$SSH $FIRST_ADDED_NODE sudo pg_ctlcluster 10 main stop"
    $SSH "$FIRST_ADDED_NODE" sudo pg_ctlcluster 10 main stop

    while true; do
        if [ "$timeLoop" -gt 300 ]; then
            failure "No failover job that is finished after $timeLoop seconds."
            mys9s job --list
            break;
        fi

        if s9s job --list | \
            grep "FINISHED" | \
            grep --quiet "Failover to a New Master"; 
        then
            mys9s job --list
            success "  o The failover job is finished, ok."
            break
        fi

        sleep 5
        let timeLoop+=5
    done

    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    print_title "Dropping the Cluster"

    begin_verbatim
    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION

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
    runFunctionalTest testCreateCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddNode
    runFunctionalTest testAddNode

    runFunctionalTest testStopMaster
    
    runFunctionalTest testDrop
fi

endTests


