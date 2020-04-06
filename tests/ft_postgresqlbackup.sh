#! /bin/bash
MYNAME=$(basename "$0")
MYBASENAME=$(basename "$MYNAME" .sh)
MYDIR=$(dirname "$0")
VERSION="0.0.1"
STDOUT_FILE=ft_errors_stdout
VERBOSE=""

LOG_OPTION="--log"
DEBUG_OPTION=""

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
CLUSTER_ID_FROM_BACKUP=""
OPTION_RESET_CONFIG=""
CONTAINER_SERVER=""
PROVIDER_VERSION="10"

SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"

CONTAINER_NAME1="${MYBASENAME}_01_$$"
CONTAINER_NAME2="${MYBASENAME}_02_$$"
CONTAINER_NAME3="${MYBASENAME}_03_$$"
CONTAINER_NAME4="${MYBASENAME}_04_$$"

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
 
  $MYNAME - PostgreSQL backup test script.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
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
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Creating PostgreSQL Cluster"
    begin_verbatim

    nodeName=$(create_node --autodestroy $CONTAINER_NAME1)
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
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    # Waiting until 
    wait_for_cluster_started "$CLUSTER_NAME"
    if [ $? -ne 0 ]; then
        failure "The cluster failed to start."
        mys9s cluster --stat
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "  o Cluster ID is $CLUSTER_ID, ok."
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
    end_verbatim
}

#
# Dropping the cluster from the controller.
# backup method: pg_basebackup
# verify backup: install software: true  terminate_db_server: false
# verify backup: install software: false terminate_db_server: false
#
function testCreateBackup()
{
    print_title "Taking backup of the Cluster"
    begin_verbatim

    #
    # Creating a backup.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes="${FIRST_ADDED_NODE}" \
        --backup-directory=/tmp \
        --backup-method=pg_basebackup \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    mys9s backup --list --long | tail -n1
    end_verbatim

    #
    #
    #
    print_title "Verify Backup First Run"
    begin_verbatim

    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        tail -n1 | \
        awk '{print $1}')

    nodeName=$(create_node --autodestroy $CONTAINER_NAME2)
    VERIFY_BACKUP_NODE="$nodeName:5432;"

    mys9s backup \
        --verify \
        --cluster-id=${CLUSTER_ID} \
        --backup-id=${backupId} \
        --test-server="$VERIFY_BACKUP_NODE" \
        --no-terminate \
        $LOG_OPTION \
        $DEBUG_OPTION

    end_verbatim

    #
    # 
    #
    print_title "Verify Backup Second Run"
    begin_verbatim

    mys9s backup \
        --verify \
        --cluster-id=${CLUSTER_ID} \
        --backup-id=${backupId} \
        --test-server="$VERIFY_BACKUP_NODE" \
        --no-terminate \
        --no-install \
        $LOG_OPTION \
        $DEBUG_OPTION
    end_verbatim

}

#
# Dropping the cluster from the controller.
#
function testVerifyBackup()
{
    print_title "Verifying the last created backup"
    begin_verbatim

    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        tail -n1 | \
        awk '{print $1}')

    nodeName=$(create_node --autodestroy $CONTAINER_NAME3)
    VERIFY_BACKUP_NODE="$nodeName:5432;"

    #
    # Verify the last backup.
    #
    mys9s backup \
        --verify \
        --cluster-id=${CLUSTER_ID} \
        --backup-id=${backupId} \
        --test-server="$VERIFY_BACKUP_NODE" \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    mys9s backup --list --long | tail -n1
    end_verbatim
}

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
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    end_verbatim
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

    runFunctionalTest testDrop
fi

endTests


