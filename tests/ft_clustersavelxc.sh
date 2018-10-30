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

PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

OPTION_INSTALL=""
OPTION_NUMBER_OF_NODES="1"
PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

# The IP of the node we added first and last. Empty if we did not.
CLUSTER_NODES=""
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""
OUTPUT_DIR="${HOME}/cmon-saved-clusters"
OUTPUT_FILE="${MYBASENAME}_$$.tgz"

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
 
  $MYNAME - Test script for s9s to check cluster save/restore features. 

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --leave-nodes    Do not destroy the nodes at exit.
  
  --provider-version=VERSION The SQL server provider version.
  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:
  o createController     Creates a second controller in a container.
  o testCreateCluster    Creates a cluster to be saved.
  o testSaveCluster      Saves the cluster on the local controller.
  o testRestoreCluster   Loads the cluster on the remote controller.
  o cleanup              Cleans up previously allocated resources.

EXAMPLE
 ./$MYNAME --print-commands --server=core1 --reset-config --install

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

        --number-of-nodes)
            shift
            OPTION_NUMBER_OF_NODES="$1"
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

    check_container_server \
        --server-name "$CONTAINER_SERVER" \
        --class       "CmonLxcServer"
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local node_ip
    local exitCode
    local node_serial=1
    local node_name

    print_title "Creating a Galera Cluster"
    
    while [ "$node_serial" -le "$OPTION_NUMBER_OF_NODES" ]; do
        node_name=$(printf "${MYBASENAME}_node%03d_$$" "$node_serial")

        if [ -n "$CLUSTER_NODES" ]; then
            CLUSTER_NODES+=";"
        fi

        CLUSTER_NODES+="$node_name"

        if [ -z "$FIRST_ADDED_NODE" ]; then
            FIRST_ADDED_NODE="$node_ip"
        fi

        let node_serial+=1
    done
     
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --vendor="$OPTION_VENDOR" \
        --cloud=lxc \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        --nodes="$CLUSTER_NODES" \
        --containers="$CLUSTER_NODES" \
        --generate-key \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
        exit 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
}

function testSaveCluster()
{
    local tgz_file="$OUTPUT_DIR/$OUTPUT_FILE"

    print_title "Saving Cluster"
    mys9s backup \
        --save-cluster \
        --cluster-id=1 \
        --backup-directory=$OUTPUT_DIR \
        --output-file=$OUTPUT_FILE \
        --log
    
    check_exit_code $?

    if [ ! -f "$tgz_file" ]; then
        failure "File '$tgz_file' was not created."
    else
        success "  o File '$tgz_file' was created, ok"
    fi
}

function testDropCluster()
{
    print_title "Dropping Original Cluster"
    cat <<EOF
Dropping the original cluster from the controller so that we can restore it from
the saved file.

EOF

    mys9s cluster \
        --drop \
        --cluster-id=1 \
        --log

    check_exit_code $?

    ls -lha /tmp/cmon_keys
    tree /tmp/cmon_keys

    rm -rvf /tmp/cmon_keys
}

function testRestoreCluster()
{
    local local_file="$OUTPUT_DIR/$OUTPUT_FILE"

    print_title "Restoring Cluster on $SECONDARY_CONTROLLER_IP"

    # Restoring the cluster on the remote controller.
    s9s backup \
        --restore-cluster \
        --input-file=$local_file \
        --debug \
        --log

    check_exit_code $?


    #
    # The cluster is now restored and should be soon in started state.
    #
    print_title "Waiting until Cluster $CLUSTER_NAME is Started"
    wait_for_cluster_started "$CLUSTER_NAME"
    check_exit_code_no_job $?
    
    mys9s cluster \
        --stat \
        --cmon-user=system \
        --password=secret 
    
    mys9s node \
        --list \
        --long \
        --cmon-user=system \
        --password=secret 
    
    ls -lha /tmp/cmon_keys
    tree /tmp/cmon_keys
}

function cleanup()
{
    local node

    print_title "Cleaning Up"
    cat <<EOF
Freeing previously allocated resources at the end of the test. Most importantly
deleting created virtual machines.
EOF

    #
    # Dropping the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=2 \
        --log

    #
    # Deleting the output file.
    #
    if [ -f "$OUTPUT_DIR/$OUTPUT_FILE" ]; then
        rm -f "$OUTPUT_DIR/$OUTPUT_FILE"
    fi

    #
    # Deleting the containers.
    #
    for node in $(echo $CLUSTER_NODES | tr ';' ' '); do
        mys9s container --delete --wait $node
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
        runFunctionalTest testCreateCluster
        runFunctionalTest testSaveCluster
        runFunctionalTest testRestoreCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest testCreateCluster
    runFunctionalTest testSaveCluster
    runFunctionalTest testDropCluster
    runFunctionalTest testRestoreCluster
    runFunctionalTest cleanup
fi

endTests

