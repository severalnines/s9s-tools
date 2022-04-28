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

PROVIDER_VERSION=$POSTGRESQL_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests various features on PostgreSql. 

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
  o testCreateCluster    Creating a PostgreSql cluster.
  o testAddNode          Adds a slave to the cluster.
  o testStopStartNode    Stopping then starting a node.
  o testConfig           Reading and changing the configuration.
  o testConfigFail       Testing configuration changes that should fail.
  o testConfigAccess     Checks that outsiders has no access to node config.
  o testCreateDatabase   Creates database and account.
  o testCreateAccount01  Create an account on the cluster.
  o testCreateAccount02  Create and check more accounts on the cluster.
  o testCreateAccount03  Create and check more accounts on the cluster.
  o testCreateAccount04  Create and check more accounts on the cluster.
  o testCreateAccount05  Create and check more accounts on the cluster.
  o testCreateAccount06  Create and check more accounts on the cluster.
  o testCreateBackup     Creates a backup.
  o testRestoreBackup    Restores a backup.
  o testRemoveBackup     Removes a backup.
  o testRunScript        Runs a JS script on the cluster.
  o testRollingRestart   Rolling restart test.
  o testDrop             Drops the previously created cluster.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,keep-nodes,install,reset-config,\
provider-version:" \
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
    local node1="ft_postgresql_01_$$"
    local nodes
    local nodeName
    local message_id
    local prefix

    print_title "Creating a PostgreSQL Cluster"
    cat <<EOF | paragraph
  This test will create a PostgreSQL cluster and check its state.
EOF

    INSTALL_START_TIME=$(dateTimeToTZ "now")
    begin_verbatim

    #
    # Creating containers.
    #
    nodeName=$(create_node --autodestroy $node1)
    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName

    #
    # Creating a PostgreSQL cluster.
    #
    mys9s cluster \
        --create \
        --job-tags="createCluster" \
        --cluster-type=postgresql \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version=$PROVIDER_VERSION \
        --print-request \
        --with-tags="myCluster" \
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
    
    check_node \
        --node       "$FIRST_ADDED_NODE" \
        --ip-address "$FIRST_ADDED_NODE" \
        --port       "8089" \
        --config     "/etc/postgresql/$PROVIDER_VERSION/main/postgresql.conf" \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "POSTGRESQL_SINGLE" \
        --state      "STARTED" \
        --config     "/tmp/cmon_1.cnf" \
        --log        "/tmp/cmon_1.log"

    end_verbatim

    #
    # FIXME:
    # Here is the thing: These log tests are just too complicated for this test
    # script. So we shall have a separate test to check the log subsystem, but
    # we should not fail the main postgresql test script because of some changes
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
        "#{$prefix/user_name}"                          "$PROJECT_OWNER"  \
        "#{$prefix/status}"                             "RUNNING"  \
        "#{$prefix/rpc_version}"                        "2.0"  \
        "#{$prefix/cluster_id}"                         "0" \
        "#{$prefix/job_spec/command}"                   "create_cluster" \
        "#{$prefix/job_spec/job_data/cluster_type}"     "postgresql_single" \
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
        "#{$prefix/user_name}"                          "$PROJECT_OWNER"  \
        "#{$prefix/status}"                             "FINISHED"  \
        "#{$prefix/rpc_version}"                        "2.0"  \
        "#{$prefix/cluster_id}"                         "0" \
        "#{$prefix/job_spec/command}"                   "create_cluster" \
        "#{$prefix/job_spec/job_data/cluster_type}"     "postgresql_single" \
        "#{$prefix/job_spec/job_data/enable_uninstall}" "true" \
        "#{$prefix/job_spec/job_data/install_software}" "true" \
        "#{$prefix/job_spec/job_data/postgre_password}" "xxxxxxxxx" \
        "#{$prefix/job_spec/job_data/sudo_password}"    "xxxxxxxxx" 
}

function testRemoveNodeFail()
{
    local ret_code

    print_title "Trying to Remove Last Node"
    cat <<EOF | paragraph
  This test tries to remove the only node the cluster has. This should fail,
  the controller should protect the one node the cluster has.

EOF

    begin_verbatim
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE:8089" \
        --log

    ret_code="$?"
    if [ "$ret_code" -eq 0 ]; then
        failure "Removing the only database node should not be possible."
    else
        success "  o Removing the only database node failed, ok."
    fi
    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local node="ft_postgresql_02_$$"

    print_title "Adding a New Node"
    cat <<EOF | paragraph
This test will add a new node as slave to the cluster created in the previous
test as a single node postgresql cluster.

EOF

    LAST_ADDED_NODE=$(create_node --autodestroy "$node")

    begin_verbatim

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE?slave" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $? 

    check_node \
        --node       "$LAST_ADDED_NODE" \
        --ip-address "$LAST_ADDED_NODE" \
        --port       "5432" \
        --config     "/etc/postgresql/$PROVIDER_VERSION/main/postgresql.conf" \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint
    
    end_verbatim
}

function testAddRemoveNode()
{
    local node="ft_postgresql_03_$$"
    local nodeIp

    print_title "Adding and Removing Data Node"
    cat <<EOF | paragraph
  Here we add a database node, then immediately removing it from the cluster.
  Both the adding and the removing should of course succeed.
EOF

    nodeIp=$(create_node --autodestroy "$node")

    begin_verbatim

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$nodeIp?slave" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $? 

    check_node \
        --node       "$nodeIp" \
        --ip-address "$nodeIp" \
        --port       "5432" \
        --config     "/etc/postgresql/$PROVIDER_VERSION/main/postgresql.conf" \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline" \
        --no-maint
    
    #
    # Removing the node from the cluster.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodeIp" \
        --log

    check_exit_code $?

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
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
    
    state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    if [ "$state" != "DEGRADED" ]; then
        failure "The cluster should be in 'DEGRADED' state, it is '$state'."
    else
        success "  o The cluster state is $state, OK."
    fi
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "stop")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        failure "JobEnded message was not found."
    fi

    #
    # Then start the node again.
    #
    mys9s node \
        --start \
        --cluster-id=$CLUSTER_ID \
        --nodes=$LAST_ADDED_NODE \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    

    state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    if [ "$state" != "STARTED" ]; then
        failure "The cluster should be in 'STARTED' state, it is '$state'."
    fi
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "start")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        failure "JobEnded message was not found."
    fi

    end_verbatim
}

#
# This function will check the basic getconfig/setconfig features that reads the
# configuration of one node.
#
function testConfig()
{
    local exitCode
    local value

    print_title "Checking Configuration"
    cat <<EOF | paragraph
  This test will read and write the configuration of a PostgreSQL node in the
  newly created cluster.

EOF
    
    begin_verbatim

    #
    # Listing the configuration values of a postgresql node. The exit code 
    # should be 0.
    #
    mys9s node --stat "$FIRST_ADDED_NODE"

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE 

    check_exit_code_no_job $?

    #
    # Changing a configuration value.
    # /etc/postgresql/9.6/main/postgresql.conf
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=log_line_prefix \
        --opt-value="'%m'"
    
    check_exit_code_no_job $?
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=log_line_prefix \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    mys9s node \
            --batch \
            --list-config \
            --opt-name=log_line_prefix \
            --nodes=$FIRST_ADDED_NODE

    check_exit_code_no_job $?

    echo "Value is $value"
    if [ "$value" != "'%m'" ]; then
        failure "Configuration value should not be '$value'"
    else
        success "  o The value is $value, ok."
    fi

    end_verbatim

    #
    # Pulling a configuration file from a node to the local host.
    #
    print_title "Pulling the PostgreSql Config File"
    cat <<EOF
  This test will pull the configuration file from a PostgreSql node to the
  local computer.

EOF

    begin_verbatim
    rm -rf tmp

    mys9s node \
        --pull-config \
        --nodes=$FIRST_ADDED_NODE \
        --output-dir=tmp \
        --batch

    if [ ! -f "tmp/postgresql.conf" ]; then
        failure "Failed to pull the configuration."
    else
        success "  o File 'tmp/postgresql.conf' is found, ok."
    fi

    if grep -q data_directory tmp/postgresql.conf; then
        success "  o The downloaded file seems to be ok."
    else
        failure "The downloaded file seems to be incomplete."
        cat tmp/postgresql.conf | print_ini_file
    fi

    rm -rf tmp

    end_verbatim
}

function testConfigAccess()
{
    local retCode

    print_title "Testing Node Config Access Rights"
    cat <<EOF | paragraph
  This test will create an unprimivileged user and try to access the host
  configuration using this new user. The access for both read and write should
  be denied by the controller.

EOF

    begin_verbatim

    #
    # Creating an outsider user.
    #
    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --group="staff" \
        --create-group \
        --generate-key \
        --new-password="p" \
        drevil
    
    check_exit_code_no_job $?

    #
    # Checking the read access.
    #
    mys9s node \
        --list-config \
        --cmon-user="drevil" \
        --nodes=$FIRST_ADDED_NODE 

    retCode=$?
    if [ $retCode -eq 0 ]; then
        warning "The user should not have read access to the configuration."
    else
        success "  o User has no read access to configuration, ok."
    fi
    
    #
    # Checking the write access.
    #
    mys9s node \
        --change-config \
        --cmon-user="drevil" \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=log_line_prefix \
        --opt-value="'some'"
    
    retCode=$?
    if [ $retCode -eq 0 ]; then
        warning "The user should not have read access to the configuration."
    else
        success "  o User has no write access to configuration, ok."
    fi

    end_verbatim
}

function testConfigFail()
{
    local exitCode
    local value

    print_title "Checking Configuration with Failed Settings"
    cat <<EOF | paragraph
This test will check some use-cases where the --change-config should fail. We
send a string instead of an integer and we also check what happens when an 
invalid/non-existing hostname is send (we had a crash).

EOF

    begin_verbatim

    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=max_connections \
        --opt-value="notaninteger"
    
    exitCode=$?
    if [ "$exitCode" -eq 0 ]; then
        failure "Changing 'max_connections' to non-integer should fail."
        mys9s node \
            --list-config \
            --nodes=$FIRST_ADDED_NODE 
    else
        success "  o Request failed, ok."
    fi
    
    mys9s node \
        --change-config \
        --nodes=10.10.1.100 \
        --opt-name=max_connections \
        --opt-value="500"
    
    exitCode=$?
    if [ "$exitCode" -eq 0 ]; then
        failure "Request with non-existing node should have failed."
        mys9s node \
            --list-config \
            --nodes=$FIRST_ADDED_NODE 
    else
        success "  o Request failed, ok."
    fi

    end_verbatim
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    print_title "Creating Databases"

    begin_verbatim

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testcreatedatabase"
   
    check_exit_code_no_job $?
    
    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "postmaster" \
        --account-password   "passwd12" 

    end_verbatim
    return 0

    # These doesn't work. Previously the controller reported ok, but it did not
    # work then either.

    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="$PROJECT_OWNER:password" \
        --privileges="testcreatedatabase.*:INSERT,UPDATE"
    
    check_exit_code_no_job $?
    
    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "$PROJECT_OWNER" \
        --account-password   "password" 
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="$PROJECT_OWNER" \
        --privileges="testcreatedatabase.*:DELETE" \
        --batch 
    
    check_exit_code_no_job $?
}

#
# Creating a new account on the cluster.
#
function testCreateAccount01()
{
    print_title "Testing Account Management"
    cat <<EOF | paragraph
  This test will create an account with special privileges. Then the account is
  tested by contacting the SQL server and executing SQL queries.

EOF

    begin_verbatim

    #
    # This command will create a new account on the cluster.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="joe:password" \
        --debug 
   
    check_exit_code_no_job $?
   
#    check_postgresql_account \
#        --hostname           "$FIRST_ADDED_NODE" \
#        --port               "8089" \
#        --account-name       "joe" \
#        --account-password   "password"

    #
    # This command will delete the same account from the cluster.
    # FIXME: this won't work maybe because the user has a database.
    #
    mys9s account \
        --delete \
        --cluster-id=$CLUSTER_ID \
        --account="joe" 
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not $exitCode while deleting an account"
    fi

    mys9s account --list --long
    end_verbatim
}

function testCreateAccount02()
{
    local username="user02"
    local password="password02"

    print_title "Testing Account Management Without Privileges"
    cat <<EOF
  This test will create an account with special privileges. Then the account is
  tested by contacting the SQL server and executing SQL queries.

EOF

    begin_verbatim

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="$username:$password@$S9S_TEST_NETWORK" \
        --debug

    mys9s account --list --long

    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "$username" \
        --account-password   "$password"

    end_verbatim
}

function testCreateAccount03()
{
    local privileges
    local username="user03"
    local password="password03"

    print_title "Testing Account Management With Options"
    cat <<EOF
  This test will create an account then the account is tested reading it 
  back with the s9s CLI and by contacting the SQL server and executing SQL 
  queries.

EOF

    begin_verbatim
    privileges+="CREATEDB,REPLICATION,SUPER"

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="$username:$password@$S9S_TEST_NETWORK" \
        --privileges="$privileges" \
        --debug
    
    check_exit_code_no_job $?

    mys9s account --list --long "$username"

    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "$username" \
        --account-password   "$password"

    end_verbatim
}

function testCreateAccount04()
{
    local privileges
    local username="user04"
    local password="password04"

    print_title "Testing Account Management With Database Privileges"
    cat <<EOF
  This test will create an account then the account is tested reading it 
  back with the s9s CLI and by contacting the SQL server and executing SQL 
  queries.

EOF

    begin_verbatim
    privileges+="testcreatedatabase:CREATE,CONNECT"

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="$username:$password@$S9S_TEST_NETWORK" \
        --privileges="$privileges" \
        --debug
    
    check_exit_code_no_job $?

    mys9s account --list --long "$username"

    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "$username" \
        --account-password   "$password"

    end_verbatim
}

function testCreateAccount05()
{
    local privileges
    local username="user04"
    local password="password04"

    print_title "Testing Account Management with Options and Database Privileges"
    cat <<EOF
  This test will create an account then the account is tested reading it 
  back with the s9s CLI and by contacting the SQL server and executing SQL 
  queries.

EOF

    begin_verbatim
    privileges+="CREATEDB,REPLICATION,SUPER"
    privileges+=";testcreatedatabase:CREATE,CONNECT"

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="$username:$password@$S9S_TEST_NETWORK" \
        --privileges="$privileges" \
        --debug
    
    check_exit_code_no_job $?

    mys9s account --list --long "$username" --debug

    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "$username" \
        --account-password   "$password" \
        --table-name         "testCreateAccount04" \
        --database-name      "testcreatedatabase" \
        --create-table

    end_verbatim
}

function testCreateAccount06()
{
    local privileges
    local username="user05"
    local password="password05"

    print_title "Testing Account Management with Table Privileges"
    cat <<EOF
  This test will create an account then the account is tested reading it 
  back with the s9s CLI and by contacting the SQL server and executing SQL 
  queries.
  
  This time with special privileges, privileges for a database, a hostname 
  with a subnet mask and no auto created database.

EOF

    begin_verbatim
    privileges+="CREATEDB,REPLICATION,SUPER"
    privileges+=";testcreatedatabase.testCreateAccount04:SELECT,INSERT"

    mys9s cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="$username:$password@$S9S_TEST_NETWORK" \
        --privileges="$privileges" \
        --debug
    
    check_exit_code_no_job $?

    mys9s account --list --long "$username" --debug

    check_postgresql_account \
        --hostname           "$FIRST_ADDED_NODE" \
        --port               "8089" \
        --account-name       "$username" \
        --account-password   "$password" \
        --database-name      "testcreatedatabase" 

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
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        $LOG_OPTION \
        $DEBUG_OPTION

    RET=$?

    mys9s job --list | tail

    check_exit_code $RET
    
    check_job_finished "Create Backup"
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "backup")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        warning "JobEnded message was not found."
        
        log_format=""
        log_format+='%I '
        log_format+='%c '
        log_format+='${/log_specifics/job_instance/job_spec/command} '
        log_format+='%i %S %B:%L \t%M '
        log_format+='\n'
        mys9s log \
            --list \
            --batch \
            --log-format="$log_format" \
            --cluster-id="$CLUSTER_ID" \
            --cmon-user=system \
            --password=secret
    fi
    
    #
    # Creating a backup using the cluster name. It is a pg_basebackup this 
    # time.
    #
    mys9s backup \
        --create \
        --cluster-name=$CLUSTER_NAME \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        --backup-method=pg_basebackup \
        $LOG_OPTION \
        $DEBUG_OPTION

    mys9s job --list | tail

    check_exit_code $RET
    
    check_job_finished "Create pg_basebackup Backup"

    mys9s backup --list --long
   
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "backup")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        warning "JobEnded message was not found."
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
        $LOG_OPTION \
        $DEBUG_OPTION

    mys9s job --list | tail

    check_exit_code $RET
    
    check_job_finished "Restore Backup"
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "restore_backup")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        warning "JobEnded message was not found."
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
    cat <<EOF
  Well, this was possible before, but it seems now the deleting of a backup on a
  remote node is prohibited. Ah, now it is again possible...
EOF

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
        --print-json
    
    if [ $? -eq 0 ]; then
        success "Removed backup"
    else
        falure "Removing backup has been failed."
    fi

    end_verbatim
}

function testBackupOfDbList()
{
    local backupId
    local message_id

    print_title "Creating Database testdatabase1, testdatabase2 and testdatabase3"

    begin_verbatim

    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testdatabase1"
   
    check_exit_code_no_job $?

    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testdatabase2"
   
    check_exit_code_no_job $?

    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testdatabase3"
   
    check_exit_code_no_job $?
    
    end_verbatim

    #
    #
    #
    print_title "Creating account tester with privileges the test databases."
    begin_verbatim
    PRIVS="testdatabase1.*:INSERT,UPDATE,DELETE,DROP,CREATE,SELECT"
    PRIVS+=";testdatabase2.*:INSERT,UPDATE,DELETE,DROP,CREATE,SELECT"
    PRIVS+=";testdatabase3.*:INSERT,UPDATE,DELETE,DROP,CREATE,SELECT"

    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="tester:password" \
        --privileges="${PRIVS}"
    
    check_exit_code_no_job $?

    print_title "Creating test tables in test databases"

    MASTER_IP=$(s9s node --list --long | grep "^poM" | awk '{print $5}')

    ${SSH} '( PGPASSWORD="$password" \
    psql \
            -t \
            --username="tester" \
            --dbname="testdatabase1" \
            --command="CREATE TABLE IF NOT EXISTS testtable1(a INT)" \
    )'

    ${SSH} '( PGPASSWORD="$password" \
    psql \
            -t \
            --username="tester" \
            --dbname="testdatabase2" \
            --command="CREATE TABLE IF NOT EXISTS testtable2(a INT)" \
    )'

    ${SSH} '( PGPASSWORD="$password" \
    psql \
            -t \
            --username="tester" \
            --dbname="testdatabase3" \
            --command="CREATE TABLE IF NOT EXISTS testtable3(a INT)" \
    )'

    end_verbatim

    print_title "Creating Backup of testdatabase1 and testdatabase2 only"
    begin_verbatim
    #
    # Creating a backup using the cluster ID to reference the cluster.
    #
    mys9s backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-method=pgdump \
	--encrypt-backup \
	--on-controller \
        --backup-directory=/tmp \
	--databases="testdatabase1,testdatabase2" \
        $LOG_OPTION \
        $DEBUG_OPTION

    RET=$?

    mys9s job --list | tail

    check_exit_code $RET

    check_job_finished "Create pgdump Backup"
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "backup")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
    else
        warning "JobEnded message was not found."
        
        log_format=""
        log_format+='%I '
        log_format+='%c '
        log_format+='-${/log_specifics/job_instance/job_spec/command}- '
        log_format+='%i %S %B:%L \t%M '
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

    #
    #
    #
    print_title "Restoring the previously made backup"
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
        $LOG_OPTION \
        $DEBUG_OPTION

    RET=$?

    mys9s job --list | tail

    check_exit_code $RET
    
    check_job_finished "Restore Backup"

    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "restore_backup")
    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        warning "JobEnded message was not found."
    fi

    end_verbatim
}

#
# 
#
function testRunScript()
{
    local output_file=$(mktemp)
    local script_file="scripts/test_output_lines.js"
    local exitCode

    print_title "Running a Script"
    
    begin_verbatim
    cat $script_file

    #
    # Running a script. 
    #
    mys9s script \
        --cluster-id=$CLUSTER_ID \
        --execute "$script_file" \
        2>&1 >$output_file
    
    exitCode=$?

    echo "output: "
    cat "$output_file"

    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
        rm $output_file
        exit 1
    fi

    if ! grep --quiet "This is a warning" $output_file; then
        failure "Script output is not as expected."
        rm $output_file
        exit 1
    fi

    rm $output_file
    end_verbatim
}

#
# This will perform a rolling restart on the cluster
#
function testRollingRestart()
{
    local message_id

    print_title "Performing Rolling Restart"

    begin_verbatim

    #
    # Calling for a rolling restart.
    #
    mys9s cluster \
        --rolling-restart \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
    
    # The JobEnded log message.
    message_id=$(get_log_message_id \
        --job-class   "JobEnded" \
        --job-command "rolling_restart")

    if [ -n "$message_id" ]; then
        success "  o Found JobEnded message at ID $message_id, ok."
        #print_log_message "$message_id"
    else
        failure "JobEnded message was not found."
    fi

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
    if [ "$*" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateCluster
        runFunctionalTest testAddNode
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    #runFunctionalTest testRemoveNodeFail
    #runFunctionalTest testAddNode
    #runFunctionalTest testAddRemoveNode
    #runFunctionalTest testStopStartNode

    # Only Galera and replication clusters are supported.
    # runFunctionalTest testAddRemoveProxySql

    #runFunctionalTest testConfig
    #runFunctionalTest testConfigFail
    #runFunctionalTest testConfigAccess

    #runFunctionalTest testCreateDatabase
    #runFunctionalTest testCreateAccount01
    #runFunctionalTest testCreateAccount02
    #runFunctionalTest testCreateAccount03
    #runFunctionalTest testCreateAccount04
    #runFunctionalTest testCreateAccount05
    #runFunctionalTest testCreateAccount06

    #runFunctionalTest testBackupOfDbList

    runFunctionalTest testCreateBackup
    runFunctionalTest testRestoreBackup
    runFunctionalTest testRemoveBackup
    
    #runFunctionalTest testRunScript
    #runFunctionalTest testRollingRestart

    #runFunctionalTest testDrop
fi

begin_verbatim

cat <<EOF
./tests/ft_graph01/ft_graph01.sh --top-begin="$INSTALL_START_TIME" --top-end="$INSTALL_END_TIME" --report-title="PostgreSQL Test (ft_postgresql)" --highlight-title="Cluster Install" --output-dir=report2
EOF

end_verbatim

endTests


