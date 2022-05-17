#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.4"

LOG_OPTION="--wait"
DEBUG_OPTION=""

SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

INSTALL_START_TIME=""
INSTALL_END_TIME=""

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh

#ElasticSearch_DB_ADMIN_PASSWD=$(generate_strong_password)
SNAPSHOT_LOCATION="/mnt/data"
SNAPSHOT_REPO="cc_snapshots"
ElasticSearch_DB_ADMIN_PASSWD="myPassword"

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests various features on ElasticSearch. 

 -h, --help          Print this help and exit.
 --verbose           Print more messages.
 --log               Print the logs while waiting for the job to be ended.
 --server=SERVER     The name of the server that will hold the containers.
 --print-commands    Do not print unit test info, print the executed commands.
 --keep-nodes        Do not destroy the nodes at exit.
 --install           Just install the cluster and exit.
 --reset-config      Remove and re-generate the ~/.s9s directory.
 
 --provider-version=STRING The Elastisearch provider version.

SUPPORTED TESTS:
  o testCreateCluster    Creating a elasticsearch_single cluster.

  

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,keep-nodes,install,reset-config, --" \
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

        --keep-nodes)
            shift
            OPTION_KEEP_NODES="true"
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
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local node1="ft_elasticsearch_01_$$"
    local nodes
    local nodeName
    local message_id
    local prefix

    print_title "Creating a ElasticSearch Cluster"
    cat <<EOF | paragraph
  This test will create a ElasticSearch cluster and check its state.
EOF

    INSTALL_START_TIME=$(dateTimeToTZ "now")
    begin_verbatim

    #
    # Creating containers.
    #
    nodeName=$(create_node --autodestroy --os-vendor=ubuntu --os-release=focal $node1)
    nodes+="$node1"
    nodes+="?roles=master-data"
    FIRST_ADDED_NODE=$node1

    #
    # Creating a ElasticSearch cluster.
    #
    mys9s cluster \
        --create \
        --job-tags="createCluster" \
        --snapshot-location=$SNAPSHOT_LOCATION \
        --snapshot-repository=$SNAPSHOT_REPO \
        --cluster-type=elastic \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="admin" \
        --db-admin-passwd="$ElasticSearch_DB_ADMIN_PASSWD" \
        --vendor="elasticsearch" \
        --provider-version=8.x \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION


    check_exit_code $?
    end_verbatim

    #
    #
    #
    print_title "Waiting until the Cluster Started"

    begin_verbatim
    wait_for_cluster_started "$CLUSTER_NAME"
    if [ $? -ne 0 ]; then
        failure "The cluster failed to start."
        mys9s cluster --stat
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    INSTALL_END_TIME=$(dateTimeToTZ "now")

    mys9s cluster --stat
    mys9s node    --stat

    #
    # Checking the controller, the node and the cluster.
    #
    check_controller \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "elasticsearch" \
        --state      "STARTED" \

    end_verbatim

    #
    # FIXME:
    # Here is the thing: These log tests are just too complicated for this test
    # script. So we shall have a separate test to check the log subsystem, but
    # we should not fail the main elasticsearch test script because of some changes
    # in the log code.
    #
    # We keep the code for now, then we can move it to a separate script.
    #
    return 0

}


#
# This will create a backup.
#
function testCreateBackup()
{
    exit 0
    local message_id

    print_title "Creating Backups"

    begin_verbatim
    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE:9200" \
        --backup-directory=testSnapRepository \
        --backup-method=elasticsearch \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "backup")
    echo "Check: JobEnded message id is: $message_id"

    #if [ -n "$message_id" ]; then
    #    success "  o Found JobEnded message at ID $message_id, ok."
    #else
    #    failure "JobEnded message was not found."
    #    
    #    log_format=""
    #    log_format+='%I '
    #    log_format+='%c '
    #    log_format+='${/log_specifics/job_instance/job_spec/command} '
    #    log_format+='\n'
    #    mys9s log \
    #        --list \
    #        --batch \
    #        --log-format="$log_format" \
    #        --cluster-id="$CLUSTER_ID" \
    #        --cmon-user=system \
    #        --password=secret
    #fi

    ## TODO: verify backup

    end_verbatim
}



#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    if [ "$*" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateCluster
        #runFunctionalTest testUploadData
        #runFunctionalTest testCreateBackup
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    #runFunctionalTest testUploadData
    #runFunctionalTest testCreateBackup
    #runFunctionalTest testVerifyBackup
    #runFunctionalTest testRestoreBackup
    #runFunctionalTest testRemoveBackup

fi


endTests
