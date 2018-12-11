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
CLUSTER_ID_FROM_BACKUP=""
OPTION_RESET_CONFIG=""
CONTAINER_SERVER=""
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"
PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

VERIFY_BACKUP_NODE=""

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Galera backup test script.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.

  --provider-version=VERSION The SQL server provider version.

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

        --reset-config)
            shift
            OPTION_RESET_CONFIG="true"
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
    esac
done

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    pip-say "The test to create Galera cluster is starting now."
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName"
    FIRST_ADDED_NODE=$nodeName
    
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --vendor="$OPTION_VENDOR" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
}

#
# Dropping the cluster from the controller.
#
function testCreateBackup()
{
    print_title "Taking backup of the Cluster"

    #
    # Creating a backup.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes="${FIRST_ADDED_NODE}" \
        --backup-directory=/tmp \
        $LOG_OPTION

    check_exit_code $?

    mys9s backup --list --long | tail -n1
}

#
# Dropping the cluster from the controller.
#
function testVerifyBackup()
{
    print_title "Verifying the last created backup"

    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        tail -n1 | \
        awk '{print $1}')

    nodeName=$(create_node --autodestroy)
    VERIFY_BACKUP_NODE="$nodeName"

    #
    # Verify the last backup.
    #
    mys9s backup \
        --verify \
        --cluster-id=${CLUSTER_ID} \
        --backup-id=${backupId} \
        --test-server="$VERIFY_BACKUP_NODE" \
        $LOG_OPTION

    check_exit_code $?

    mys9s backup --list --long | tail -n1
}

function testCreateClusterFromBackup()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Creating Galera cluster from backup."

    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        tail -n1 | \
        awk '{print $1}')

    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName:8089;"
    
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --cluster-name="${CLUSTER_NAME}-frombackup" \
        --vendor="$OPTION_VENDOR" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID_FROM_BACKUP=$(find_cluster_id "${CLUSTER_NAME}-frombackup")
    if [ "$CLUSTER_ID_FROM_BACKUP" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID_FROM_BACKUP"
    else
        failure "Cluster ID '$CLUSTER_ID_FROM_BACKUP' is invalid"
    fi
}

function testDrop()
{
    print_title "Dropping the Cluster"

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION

    check_exit_code $?
}

function testDropFromBackup()
{
    print_title "Dropping the Cluster created from backup"

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID_FROM_BACKUP \
        $LOG_OPTION

    check_exit_code $?
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster

    runFunctionalTest testCreateBackup
    #runFunctionalTest testVerifyBackup

    runFunctionalTest testCreateClusterFromBackup
    runFunctionalTest testDropFromBackup

    runFunctionalTest testDrop
fi

if [ "$FAILED" == "no" ]; then
    echo "The test script is now finished. No errors were detected."
else
    echo "The test script is now finished. Some failures were detected."
fi

endTests


