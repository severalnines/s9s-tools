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
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""
SECONDARY_CONTROLLER_IP=""
SECONDARY_CONTROLLER_URL=""
SSH_PID=""
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
# Pings the controller to check if it is up.
#
function testPing()
{
    print_title "Pinging Controller."

    #
    # Pinging. 
    #
    mys9s cluster --ping 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
    fi
}

function ssh_to_controller()
{
    echo "Executing ssh \"$SECONDARY_CONTROLLER_IP\" bash --login -c \"$*\""
    ssh -o ConnectTimeout=1 \
        -o UserKnownHostsFile=/dev/null \
        -o StrictHostKeyChecking=no \
        -o LogLevel=quiet \
        "$SECONDARY_CONTROLLER_IP" \
        -- "bash --login -c \"$*\""
}

function createController()
{
    local node_name="ft_clustersave_00_$$"
    local sdir="~/clustercontrol-enterprise"
    local tdir="$sdir/tests"

    print_title "Creating Secondary Controller"
    node_ip=$(create_node --autodestroy --template "ubuntu-s9s" "$node_name") 
    SECONDARY_CONTROLLER_IP="$node_ip"
    SECONDARY_CONTROLLER_URL="https://$SECONDARY_CONTROLLER_IP:9556"

    print_title "Pulling Source on $SECONDARY_CONTROLLER_IP"
    ssh_to_controller "cd $sdir && git pull"
    if [ $? -ne 0 ]; then
        failure "Failed to git pull."
        return 1
    fi
    
    print_title "Running autogen.sh on $SECONDARY_CONTROLLER_IP"
    ssh_to_controller "cd $sdir && env | grep PATH"
    ssh_to_controller "cd $sdir && ./autogen.sh"
    if [ $? -ne 0 ]; then
        failure "Failed to configure."
        while true; do 
            sleep 1000
        done

        return 1
    fi

    print_title "Compiling Source"
    ssh_to_controller "cd $sdir && make -j15"
    if [ $? -ne 0 ]; then
        failure "Failed to compile."
        return 1
    fi
    
    ssh_to_controller "rm -rvf /var/tmp/cmon* /tmp/cmon*"

    #
    # Starting the secondary controller.
    #
    print_title "Starting Secondary Controller"
    rm -f nohup.out

    nohup ssh \
        -o ConnectTimeout=1 \
        -o UserKnownHostsFile=/dev/null \
        -o StrictHostKeyChecking=no \
        -o LogLevel=quiet \
        $SECONDARY_CONTROLLER_IP \
        -- ~/clustercontrol-enterprise/tests/runftfull.sh &
    
    SSH_PID=$!

    if [ $? -ne 0 ]; then
        failure "Failed to start controller."
        return 1
    fi

    for n in $(seq 1 120); do
        if grep --quiet testWait "nohup.out"; then
            break
        fi
        sleep 1
    done

    cat "nohup.out"
    
    print_title "Testing Connection to the New Controller"
    mys9s user \
        --list --long \
        --controller=$SECONDARY_CONTROLLER_URL \
        --cmon-user=system \
        --password=secret
    
    check_exit_code_no_job $?

    return 0
    while true; do
        sleep 10
        echo -n "."
    done
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local node_ip
    local exitCode
    local node_serial=1
    local node_name

    print_title "Creating a Galera Cluster"
    
    while [ "$node_serial" -le "$OPTION_NUMBER_OF_NODES" ]; do
        node_name=$(printf "${MYBASENAME}_node%03d_$$" "$node_serial")

        echo "Creating node #$node_serial"
        node_ip=$(create_node --autodestroy "$node_name")

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$node_ip"

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
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
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

function testRestoreCluster()
{
    local local_file="$OUTPUT_DIR/$OUTPUT_FILE"
    local remote_file="/tmp/ft_clustersave_$$.tar.gz"

    print_title "Restoring Cluster on $SECONDARY_CONTROLLER_IP"

    # Copying the tar.gz file to the secondary controller.
    scp "$local_file" "$SECONDARY_CONTROLLER_IP:$remote_file"
    check_exit_code_no_job $?
    
    # Restoring the cluster on the remote controller.
    s9s backup \
        --restore-cluster \
        --input-file=$remote_file \
        --controller=$SECONDARY_CONTROLLER_URL \
        --cmon-user=system \
        --password=secret \
        --log

    check_exit_code $?


    #
    #
    #
    print_title "Waiting until Cluster $CLUSTER_NAME is Started"
    wait_for_cluster_started "$CLUSTER_NAME"
    
    s9s cluster \
        --stat \
        --controller=$SECONDARY_CONTROLLER_URL \
        --cmon-user=system \
        --password=secret 
    
    s9s node \
        --list \
        --long \
        --controller=$SECONDARY_CONTROLLER_URL \
        --cmon-user=system \
        --password=secret 
}

function cleanup()
{
    print_title "Cleaning Up"

    echo "PID for ssh: $SSH_PID"
    if [ -n "$SSH_PID" ]; then
        kill $SSH_PID
        sleep 3
        kill -9 $SSH_PID
    fi

    if [ -f "$OUTPUT_DIR/$OUTPUT_FILE" ]; then
        rm -f "$OUTPUT_DIR/$OUTPUT_FILE"
    fi
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
        runFunctionalTest testCreateCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createController
    runFunctionalTest testCreateCluster
    runFunctionalTest testSaveCluster
    runFunctionalTest testRestoreCluster
    runFunctionalTest cleanup
fi

endTests

