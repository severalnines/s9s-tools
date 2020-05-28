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
OUTPUT_DIR="${HOME}/cmon-saved-clusters"
OUTPUT_FILE="${MYBASENAME}_$$.tgz"
CLUSTER_TYPE="galera"
#CLUSTER_TYPE="postgresql"

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
  --galera         The test cluster should be a galera cluster (default).
  --postgres       The test cluster should be a postgresql cluster.
  
  --provider-version=VERSION The SQL server provider version.
  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:
  o testCreateGalera     Creates a Galera cluster to be saved.
  o testCreatePostgre    Creates a PostgreSQL cluster as an alternaive.
  o testSaveCluster      Saves the cluster on the local controller.
  o testDropCluster      Drops the original cluster from the controller.
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
galera,postgres,\
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

        --galera)
            shift
            CLUSTER_TYPE="galera"
            ;;

        --postgres)
            shift
            CLUSTER_TYPE="postgresql"
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
# This test will allocate a few nodes and install a new cluster.
#
function testCreateGalera()
{
    local nodes
    local node_ip
    local exitCode
    local node_serial=1
    local node_name
    local config_file

    #
    #
    #
    print_title "Creating a Galera Cluster"

    if [ "$CLUSTER_TYPE" != "galera" ]; then
        cat <<EOF | paragraph
  This test is not executed, an other cluster type is tested here.
EOF
        return 0
    fi

    begin_verbatim

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
        --generate-key \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
        return 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"

    config_file="/tmp/cmon_1.cnf"
    end_verbatim

    #print_subtitle "Cluster Config in $config_file"
    #cat $config_file
}


function testCreatePostgre()
{
    local nodes
    local node_ip
    local exitCode
    local node_serial=1
    local node_name

    #
    #
    #
    print_title "Creating a PostgreSQL Cluster"
    if [ "$CLUSTER_TYPE" != "postgresql" ]; then
        cat <<EOF | paragraph
  This test is not executed, an other cluster type is tested here.
EOF
    fi

    begin_verbatim
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
    # Creating a PostgreSql cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="9.5" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --generate-key \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
        return 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
}

function testSaveClusterFail()
{
    local tgz_file="$OUTPUT_DIR/$OUTPUT_FILE"
    local retcode

    print_title "Trying to Save Cluster with no ID"
    cat <<EOF
  This test will try to save a cluster while not passing the cluster ID as a 
  command line option. We had a segfault reported in the controller, so we
  have this test to check.
EOF

    begin_verbatim
    mys9s backup \
        --save-cluster-info \
        --backup-directory=$OUTPUT_DIR \
        --output-file=$OUTPUT_FILE \
        --log
    
    retcode=$?

    if [ $retcode -eq 0 ]; then
        failure "Job should not be a success."
    else
        success "  o Job failed (retcode=$retcode), ok."
    fi

    if [ ! -f "$tgz_file" ]; then
        success "  o File '$tgz_file' was not created, ok"
    else
        failure "File '$tgz_file' was created with no cluster ID."
    fi

    end_verbatim
}

function testSaveCluster()
{
    local tgz_file="$OUTPUT_DIR/$OUTPUT_FILE"

    print_title "Saving Cluster"

    begin_verbatim
    mys9s backup \
        --save-cluster-info \
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
    end_verbatim
}

function testDropCluster()
{
    print_title "Dropping Original Cluster"
    begin_verbatim
    cat <<EOF
  Dropping the original cluster from the controller so that we can restore it
  from the saved file.
EOF

    mys9s cluster \
        --drop \
        --cluster-id=1 \
        $LOG_OPTION

    check_exit_code $?
    end_verbatim
}

function testRestoreCluster()
{
    local local_file="$OUTPUT_DIR/$OUTPUT_FILE"
    local retcode
    local config_file

    print_title "Restoring Cluster"
    begin_verbatim

    # Restoring the cluster on the remote controller.
    s9s backup \
        --restore-cluster-info \
        --input-file=$local_file \
        --cmon-user=system \
        --password=secret \
        --log

    check_exit_code $?
    end_verbatim

    #
    # Checking the cluster state after it is restored.
    #
    print_title "Waiting until Cluster $CLUSTER_NAME is Started"
    begin_verbatim

    mysleep 10
    wait_for_cluster_started --system "$CLUSTER_NAME"
    retcode=$?

    mys9s cluster \
        --stat \
        --cmon-user=system \
        --password=secret 
    
    mys9s node \
        --list \
        --long \
        --cmon-user=system \
        --password=secret 

    if [ "$retcode" -ne 0 ]; then
        failure "Cluster is not is started state."
    else
        success "  o The cluster is in started state, ok"
    fi

    end_verbatim

    #
    # Checking the configuration file.
    #
    config_file="/tmp/cmon_2.cnf"
    print_subtitle "Cluster Config in $config_file"

    begin_verbatim
    if [ -f "$config_file" ]; then
        success "  o The '$config_file' file exists, ok."
    else
        failure "File '$config_file' was not found."
    fi

    cat $config_file | print_ini_file
    end_verbatim
}

function cleanup()
{
    print_title "Cleaning Up"

    begin_verbatim
    if [ -f "$OUTPUT_DIR/$OUTPUT_FILE" ]; then
        rm -f "$OUTPUT_DIR/$OUTPUT_FILE"
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
    if [ -n "$1" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateGalera
        runFunctionalTest testCreatePostgre
        runFunctionalTest testSaveCluster
        runFunctionalTest testDropCluster
        runFunctionalTest testRestoreCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateGalera
    runFunctionalTest testCreatePostgre
    runFunctionalTest testSaveClusterFail
    runFunctionalTest testSaveCluster
    runFunctionalTest testDropCluster
    runFunctionalTest testRestoreCluster
    runFunctionalTest cleanup
fi

endTests

