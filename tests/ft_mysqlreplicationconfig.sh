#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

nodes=""


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

  $MYNAME - Test script for s9s to check various error conditions.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:" \
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

    print_title "Create MySQL Replication Cluster"
    begin_verbatim

    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
    nodeName=$(create_node --autodestroy)
    SECOND_ADDED_NODE=$nodeName
    nodes+="$nodeName;"
    
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=mysqlreplication \
        --nodes="$nodes" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=5.6 \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
    end_verbatim
}

#
# Checking the controller configuration.
#
function checkController()
{
    local controller_ip
    local lines
    local expected

    print_title "Checking Controller Config"
    begin_verbatim

    controller_ip=$(s9s node --list --long | grep "^c" | awk '{print $5}')
    if [ -z "$controller_ip" ]; then
        failure "Controller was not found."
        mys9s node --list --long
    else
        success "Controller was found."
    fi
  
    # Checking the configuration values of the controller.
    lines=$(s9s node --list-config --nodes=$controller_ip)

    expected="^-  *cdt_path  *\\/ *$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s node --list-config --nodes=$controller_ip
        exit 1
    fi
    
    expected="^-  *created_by_job  *1 *$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s node --list-config --nodes=$controller_ip
        exit 1
    fi
    
    expected="^-  *group_owner  *4 *$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s node --list-config --nodes=$controller_ip
        exit 1
    fi
    
    expected="^-  *logfile  */tmp/cmon_1.log *$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s node --list-config --nodes=$controller_ip
        exit 1
    fi

    end_verbatim
}

#
# This function will check the basic getconfig/setconfig features that reads the
# configuration of one node.
#
function testSetConfig01()
{
    local exitCode
    local value
    local newValue="200"
    local name="max_connections"

    print_title "Changing configuration '$name'"
    begin_verbatim

    #
    # Listing the configuration values. The exit code should be 0.
    #
    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        max_*

    check_exit_code_no_job $?

    #
    # Changing a configuration value.
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=$name \
        --opt-group=MYSQLD \
        --opt-value=$newValue 
    
    check_exit_code_no_job $?
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    mys9s node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE 

    check_exit_code_no_job $?

    if [ "$value" != "$newValue" ]; then
        failure "Configuration value should be $newValue not $value"
        mys9s node --list-config --nodes=$FIRST_ADDED_NODE $name
        exit 1
    fi
    end_verbatim
}

#
# This test will set a configuration value that contains an SI prefixum,
# ("54M").
#
function testSetConfig02()
{
    local exitCode
    local value
    local newValue="64M"
    local name="max_heap_table_size"

    print_title "Changing configuration '$name'"
    begin_verbatim
    
    mys9s node --list --long

    #
    # Changing a configuration value.
    #
    mys9s node \
        --change-config \
        --nodes=$FIRST_ADDED_NODE \
        --opt-name=$name \
        --opt-group=MYSQLD \
        --opt-value=$newValue
    
    check_exit_code_no_job $?
    
    #
    # Reading the configuration back. This time we only read one value.
    #
    value=$($S9S node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE |  awk '{print $3}')

    mys9s node \
            --batch \
            --list-config \
            --opt-name=$name \
            --nodes=$FIRST_ADDED_NODE

    check_exit_code_no_job $?

    if [ "$value" != "$newValue" ]; then
        failure "Configuration value should be $newValue not $value"
    fi

    mys9s node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE \
        'max*'
    
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
    runFunctionalTest checkController
    runFunctionalTest testSetConfig01
    runFunctionalTest testSetConfig02
fi

endTests


