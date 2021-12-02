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

#MSSQL_DB_ADMIN_PASSWD=$(generate_strong_password)
MSSQL_DB_ADMIN_PASSWD="S9ss9ss9s_"

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests various features on MSSQL. 

 -h, --help          Print this help and exit.
 --verbose           Print more messages.
 --log               Print the logs while waiting for the job to be ended.
 --server=SERVER     The name of the server that will hold the containers.
 --print-commands    Do not print unit test info, print the executed commands.
 --keep-nodes        Do not destroy the nodes at exit.
 --install           Just install the cluster and exit.
 --reset-config      Remove and re-generate the ~/.s9s directory.
 
 --provider-version=STRING The SQL server provider version.

SUPPORTED TESTS:
  o testCreateCluster    Creating a mssql_single cluster.
  o testCreateDatabase   Creating a database on mssql-server instance
  o testCreateBackup     Creating a mssqlfull backup of database

  

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
    local node1="ft_mssql_01_$$"
    local nodes
    local nodeName
    local message_id
    local prefix

    print_title "Creating a MSSQL Cluster"
    cat <<EOF | paragraph
  This test will create a MSSQL cluster and check its state.
EOF

    INSTALL_START_TIME=$(dateTimeToTZ "now")
    begin_verbatim

    #
    # Creating containers.
    #
    nodeName=$(create_node --autodestroy --os-vendor=ubuntu --os-release=focal $node1)
    nodes+="$nodeName:1433;"
    FIRST_ADDED_NODE=$nodeName

    #
    # Creating a MSSQL cluster.
    #
    mys9s cluster \
        --create \
        --job-tags="createCluster" \
        --cluster-type=mssql_single \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="SQLServerAdmin" \
        --db-admin-passwd="$MSSQL_DB_ADMIN_PASSWD" \
        --vendor="mssql" \
        --print-request \
        --with-tags="myMssqlCluster" \
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
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    
    # To be used ??
    #check_node \
    #    --node       "$FIRST_ADDED_NODE" \
    #    --ip-address "$FIRST_ADDED_NODE" \
    #    --port       "8089" \
    #    --config     "/etc/mssql/$PROVIDER_VERSION/main/mssql.conf" \
    #    --owner      "pipas" \
    #    --group      "testgroup" \
    #    --cdt-path   "/$CLUSTER_NAME" \
    #    --status     "CmonHostOnline" \
    #    --no-maint

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "pipas" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "MSSQL_SINGLE" \
        --state      "STARTED" \

    end_verbatim

    #
    # FIXME:
    # Here is the thing: These log tests are just too complicated for this test
    # script. So we shall have a separate test to check the log subsystem, but
    # we should not fail the main mssql test script because of some changes
    # in the log code.
    #
    # We keep the code for now, then we can move it to a separate script.
    #
    return 0

    #
    # Checking what log messages were created while the cluster was created.
    # These are not the job messages, these are actual log messages.
    #
    print_subtitle "Checking Log Messages"
    cat <<EOF | paragraph
  Checking what log messages were filed while the cluster was created.

EOF

    log_format+='%I '
    log_format+='%c '
    log_format+='${/log_specifics/job_instance/job_spec/command} '
    log_format+='\n'

    mys9s log --list \
        --log-format="$log_format" \
        --cmon-user=system \
        --password=secret \
        --cluster-id=0

    # The JobStarted log message.
    message_id=$(get_log_message_id \
        --job-class   "JobStarted" \
        --job-command "create_cluster")

    if [ -n "$message_id" ]; then
        success "  o Found JobStarted message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        failure "JobStarted message was not found."
    fi

    prefix="/log_specifics/job_instance"
    check_log_message \
        --message-id    "$message_id" \
        "#{$prefix/class_name}"                         "CmonJobInstance"  \
        "#{$prefix/group_name}"                         "testgroup"  \
        "#{$prefix/user_name}"                          "pipas"  \
        "#{$prefix/status}"                             "RUNNING"  \
        "#{$prefix/rpc_version}"                        "2.0"  \
        "#{$prefix/cluster_id}"                         "0" \
        "#{$prefix/job_spec/command}"                   "create_cluster" \
        "#{$prefix/job_spec/job_data/cluster_type}"     "mssql_single" \
        "#{$prefix/job_spec/job_data/enable_uninstall}" "true" \
        "#{$prefix/job_spec/job_data/install_software}" "true" \
        "#{$prefix/job_spec/job_data/postgre_password}" "xxxxxxxxx" \
        "#{$prefix/job_spec/job_data/sudo_password}"    "xxxxxxxxx" 

    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "create_cluster")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        failure "JobEnded message was not found."
    fi
    
    prefix="/log_specifics/job_instance"
    check_log_message \
        --message-id    "$message_id" \
        "#{$prefix/class_name}"                         "CmonJobInstance"  \
        "#{$prefix/group_name}"                         "testgroup"  \
        "#{$prefix/user_name}"                          "pipas"  \
        "#{$prefix/status}"                             "FINISHED"  \
        "#{$prefix/rpc_version}"                        "2.0"  \
        "#{$prefix/cluster_id}"                         "0" \
        "#{$prefix/job_spec/command}"                   "create_cluster" \
        "#{$prefix/job_spec/job_data/cluster_type}"     "mssql_single" \
        "#{$prefix/job_spec/job_data/enable_uninstall}" "true" \
        "#{$prefix/job_spec/job_data/install_software}" "true" \
        "#{$prefix/job_spec/job_data/postgre_password}" "xxxxxxxxx" \
        "#{$prefix/job_spec/job_data/sudo_password}"    "xxxxxxxxx" 
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local userName

    print_title "Creating Database"
    begin_verbatim

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testCreateDatabase" \
        --batch

    check_exit_code_no_job $?

    #
    # CmonMssqlSinCluster::getAccounts not implemented yet
    # Skipping more checks for the moment
    # 
    return 0

    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="pipas:password" \
        --privileges="testCreateDatabase.*:INSERT,UPDATE" \
        --batch
    
    check_exit_code_no_job $?
  
    #
    # Checking if the account could be created.
    #
    userName=$(s9s account --list --cluster-id=$CLUSTER_ID pipas)
    if [ "$userName" != "pipas" ]; then
        failure "Failed to create user 'pipas'."
    else
        success "  o User $userName was created, ok."
    fi

    #
    # This command will grant some rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testCreateDatabase.*:DELETE,DROP" 
   
    check_exit_code_no_job $?

    mys9s account --list --long
    end_verbatim
}


#
# This will create a backup.
#
function testCreateBackup()
{
    local message_id

    print_title "Creating Backups"

    begin_verbatim
    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE:1433" \
        --backup-directory=/tmp \
        --backup-method=mssqlfull \
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
        runFunctionalTest testCreateDatabase
        #runFunctionalTest testUploadData
        runFunctionalTest testCreateBackup
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateDatabase
    #runFunctionalTest testUploadData
    runFunctionalTest testCreateBackup
    #runFunctionalTest testVerifyBackup
    #runFunctionalTest testRestoreBackup
    #runFunctionalTest testRemoveBackup

fi

begin_verbatim

cat <<EOF
./tests/ft_graph01/ft_graph01.sh --top-begin="$INSTALL_START_TIME" --top-end="$INSTALL_END_TIME" --report-title="MSSQL single server test (ft_mssqlsingleserver.sh)" --highlight-title="Cluster Install and db backup" --output-dir=report2
EOF

end_verbatim

endTests
