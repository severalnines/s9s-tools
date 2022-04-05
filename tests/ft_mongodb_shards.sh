#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--wait"
DEBUG_OPTION=""
LOG_OPTION="--log"
DEBUG_OPTION="--debug"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
OPTION_INSTALL=""

OPTION_VENDOR="10gen"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

BACKUP_METHOD=mongodump

CONTAINER_NAME1="${MYBASENAME}_1_$$"
CONTAINER_NAME2="${MYBASENAME}_2_$$"
CONTAINER_NAME3="${MYBASENAME}_3_$$"
CONTAINER_NAME4="${MYBASENAME}_4_$$"
CONTAINER_NAME5="${MYBASENAME}_5_$$"
CONTAINER_NAME6="${MYBASENAME}_6_$$"

cd $MYDIR
source ./include.sh
source ./include_lxc.sh

PROVIDER_VERSION=$MONGODB_DEFAULT_PROVIDER_VERSION


# This requires tests not using 'nodeIp=$(create_node)' expression
export PRINT_PIP_COMMANDS=${S9S_TEST_PRINT_COMMANDS}

if [ "${S9S_TEST_LOG_OPTION}" != "" ]; then
    export LOG_OPTION="--log"
    export DEBUG_OPTION="--debug"
fi

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]

  $MYNAME - Test script for s9s to check the ProxySQL support.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given MongoDb vendor.
  --provider-version=STRING The database server provider version.
  --server=SERVER  Use the given server to create containers.
  --keep-nodes    Do not destroy the nodes at exit.
  --install        Just install the cluster and exit.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:,\
,keep-nodes,install,provider-version:,vendor:" \
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

        --reset-config)
            shift
            OPTION_RESET_CONFIG="true"
            ;;

        --server)
            shift
            CONTAINER_SERVER="$1"
            shift
            ;;

        --keep-nodes)
            shift
            OPTION_KEEP_NODES="true"
            ;;

        --install)
            shift
            OPTION_INSTALL="--install"
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

#if [ -z "$OPTION_RESET_CONFIG" ]; then
#    printError "This script must remove the s9s config files."
#    printError "Make a copy of ~/.s9s and pass the --reset-config option."
#    exit 6
#fi

if [ -z "$CONTAINER_SERVER" ]; then
    printError "No container server specified."
    printError "Use the --server command line option to set the server."
    exit 6
fi

#
# This is where we create a MongoDb cluster.
#
function createCluster()
{
    local return_value
    #
    # Creating a Cluster.
    #
    print_title "Creating a MongoDB Cluster on LXC"
    cat <<EOF | paragraph
  Here we create a mongodb cluster.
EOF

    begin_verbatim

    create_node --autodestroy $CONTAINER_NAME1
    nodes+="$LAST_ADDED_NODE;"

    create_node --autodestroy $CONTAINER_NAME2
    nodes+="$LAST_ADDED_NODE;"

    create_node --autodestroy $CONTAINER_NAME3
    nodes+="$LAST_ADDED_NODE?rs=replset_2;"

    create_node --autodestroy $CONTAINER_NAME4
    nodes+="$LAST_ADDED_NODE?rs=replset_2;"

    create_node --autodestroy $CONTAINER_NAME5
    nodes+="MongoCfg://$LAST_ADDED_NODE;"
    nodes+="Mongos://$LAST_ADDED_NODE;"

    create_node --autodestroy $CONTAINER_NAME6
    nodes+="MongoCfg://$LAST_ADDED_NODE;"
    nodes+="Mongos://$LAST_ADDED_NODE;"


    mys9s cluster \
        --create \
        --template="ubuntu" \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=mongodb \
        --provider-version="$PROVIDER_VERSION" \
        --vendor="$OPTION_VENDOR" \
        --cloud=lxc \
        --nodes="$nodes" \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    return_value=$?
    check_exit_code $return_value
    if [ "$return_value" -ne 0 ]; then
        end_verbatim
        return 1
    fi

    mys9s cluster --stat

    #
    # Checking if the cluster is started.
    #
    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"

    mys9s node --list --long
    mys9s node --stat
    #mys9s node --list --print-json
    end_verbatim
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    print_title "Performing Rolling Restart"
    begin_verbatim

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    wait_for_cluster_started "$CLUSTER_NAME"
    mys9s cluster --stat

    mys9s node --list --long
    mys9s node --stat
    #mys9s node --list --print-json
    end_verbatim
}

function testAddRemoveNode()
{
    local node="ft_mongodb_7_$$"
    local nodeIp

    print_title "Adding and Removing Data Node"
    cat <<EOF | paragraph
  Here we add a database node, then immediately removing it from the cluster.
  Both the adding and the removing should of course succeed.
EOF

    create_node --autodestroy "$node"
    nodeIp="$LAST_ADDED_NODE"

    begin_verbatim

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="MongoDb://$nodeIp?rs=replset_2" \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    check_node \
        --node       "$nodeIp" \
        --ip-address "$nodeIp" \
        --port       "27017" \
        --config     "/etc/mongod.conf" \
        --no-maint
        #--owner      "pipas" \
        #--group      "testgroup" \
        #--cdt-path   "/$CLUSTER_NAME" \
        #--status     "CmonHostOnline" \

    #
    # Removing the node from the cluster.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodeIp:27017" \
        --log

    check_exit_code $?

    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local node="ft_mongodb_8_$$"

    print_title "Adding a New Node"
    cat <<EOF | paragraph
This test will add a new node to the cluster.

EOF

    create_node --autodestroy "$node"

    begin_verbatim

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="MongoDb://$LAST_ADDED_NODE?rs=replset_2" \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    check_node \
        --node       "$LAST_ADDED_NODE" \
        --ip-address "$LAST_ADDED_NODE" \
        --port       "27017" \
        --config     "/etc/mongod.conf" \
        --no-maint
        #--owner      "pipas" \
        #--group      "testgroup" \
        #--cdt-path   "/$CLUSTER_NAME" \
        #--status     "CmonHostOnline" \

    end_verbatim
}

#
# This test will first call a --stop then a --start on a node. Pretty basic
# stuff.
#
function testStopStartNode()
{
    local state
    local message_id

    print_title "Stopping and Starting a Node"
    cat <<EOF | paragraph
  This test will remove a node from the cluster and check if the cluster state
  is changed. Then the cluster state should again be changed.
EOF

    begin_verbatim

    #
    # First stop the node.
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    # Give chance CmonMongoCluster::sample to run and
    # - recheck the state of the nodes
    # - update the cluster state
    # FIXME: we might have a bug here, cmon restarted the mongo node
    #sleep 30

    #state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    #if [ "$state" != "DEGRADED" ]; then
    #    failure "The cluster should be in 'DEGRADED' state, it is '$state'."
    #else
    #    success "  o The cluster state is $state, OK."
    #fi

    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "stop")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        failure "JobEnded message was not found."
        FAILED="no"
    fi

    #
    # Then start the node again.
    #
    mys9s node \
        --start \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    # Give chance CmonMongoCluster::sample to run and
    # - recheck the state of the nodes
    # - update the cluster state
    # FIXME: we might have a bug here, cmon restarted the mongo node
    #sleep 30

    #state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    #if [ "$state" != "STARTED" ]; then
    #    failure "The cluster should be in 'STARTED' state, it is '$state'."
    #fi

    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "start")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        failure "JobEnded message was not found."
        FAILED="no"
    fi

    end_verbatim
}

#
# This will create a backup.
#
function testCreateBackup()
{
    local message_id

    print_title "Creating Backup"

    begin_verbatim
    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --backup-method=$BACKUP_METHOD \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "backup")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        failure "JobEnded message was not found."
        FAILED="no"
        
        log_format=""
        log_format+='%I '
        log_format+='%c '
        log_format+='${/log_specifics/job_instance/job_spec/command} '
        log_format+='\n'
        mys9s log \
            --list \
            --batch \
            --log-format="$log_format" \
            --cluster-id="$CLUSTER_ID" \
            --cmon-user=system \
            --password=secret
    fi

    end_verbatim
}

#
# This will restore a backup. 
#
function testRestoreBackup()
{
    local backupId
    local message_id

    print_title "Restoring a Backup"

    begin_verbatim
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID | \
        head -n1 | \
        awk '{print $1}')

    #
    # Restoring the backup. 
    #
    mys9s backup \
        --restore \
        --cluster-id=$CLUSTER_ID \
        --backup-id=$backupId \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "restore_backup")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        failure "JobEnded message was not found."
        FAILED="no"
    fi

    end_verbatim
}

#
# This will remove a backup. 
#
function testRemoveBackup()
{
    local backupId

    print_title "Removing a Backup"

    begin_verbatim
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        head -n1 | \
        awk '{print $1}')

    #
    # Removing the backup. 
    #
    mys9s backup \
        --delete \
        --backup-id=$backupId \
        --batch \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    end_verbatim
}

#
# Installing PerconaBackup for MongoDb
#
function testInstallPBMAgents()
{
    local node="ft_mongodb_9_$$"

    print_title "Installing PBMAgent nodes"
    cat <<EOF
  Installing Percona Backup for MongoDb agents refered as PBMAgent nodes.
  Before doing so we will need to have shared directory. For that setting
  up an NFS server and the NFSClient nodes first.

EOF

    create_node --autodestroy "$node"
    nodeIp="$LAST_ADDED_NODE"

    print_subtitle "Installing NFSClient nodes"

    begin_verbatim

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="NFSClient://*?client_dir=/pbm-backup&server_dir=/pbm-backup&&server_hostname=$nodeIp" \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    mys9s node --list --long

    end_verbatim

    print_subtitle "Installing PBMAgent nodes"

    begin_verbatim

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="PBMAgent://*?backup_dir=/pbm-backup" \
        --print-request \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    mys9s node --list --long

    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    print_title "Dropping the Cluster"
    cat <<EOF
  We are at the end of the test script, we now drop the cluster that we created
  at the beginning of this test.

EOF

    print_subtitle "Printing the Logs"

    begin_verbatim
    s9s log --list --log-format="%4I %-14h %18c %36B:%-5L %M\n"

    #
    #
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        --print-request \
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

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest createCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createCluster

    #runFunctionalTest testRollingRestart

    runFunctionalTest testAddRemoveNode
    runFunctionalTest testAddNode
    runFunctionalTest testStopStartNode

    #BACKUP_METHOD=percona-backup-mongodb

    #runFunctionalTest testInstallPBMAgents
    #runFunctionalTest testStopPBMAgent
    #runFunctionalTest testStartPBMAgent
    #runFunctionalTest testRestartPBMAgent
    #runFunctionalTest testUnregisterPBMAgent
    #runFunctionalTest testRegisterPBMAgent
    #runFunctionalTest testUninstallPBMAgent
    #runFunctionalTest testAddMissingPBMAgent
    #runFunctionalTest testReconfigurePBMAgents
    #runFunctionalTest testReinstallPBMAgents

    #runFunctionalTest testCreateBackup
    #runFunctionalTest testCreateBackup
    #runFunctionalTest testRestoreBackup
    #runFunctionalTest testCreateBackup
    #runFunctionalTest testPITRRestoreBackup
    #runFunctionalTest testRemoveBackup

    runFunctionalTest testDrop
fi

endTests
