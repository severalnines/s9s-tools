#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait --print-request"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh

MYSQL_ROOT_PASSWORD=$(generate_strong_password)

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]

  $MYNAME - Check for registering group replication clusters.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,reset-config" \
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
            LOG_OPTION="--log --print-request"
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

reset_config


#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodeName
    local exitCode

    print_title "Creating MySQL Replication Cluster"
    begin_verbatim

    nodeName=$(create_node --autodestroy)
    NODES+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
    nodeName=$(create_node --autodestroy)
    NODES+="$nodeName;"
    
    nodeName=$(create_node --autodestroy)
    NODES+="$nodeName"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=group_replication \
        --nodes="$NODES" \
        --vendor=oracle \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin-passwd="$MYSQL_ROOT_PASSWORD" \
        --provider-version=8.0 \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)

    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "  o Cluster ID is $CLUSTER_ID, ok."
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi


    s9s cluster --list --long
    s9s node --list --long
    s9s node --list --node-format="%12R %N\n"

    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    print_title "The test to drop the cluster is starting now."
    begin_verbatim

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    check_exit_code_no_job $?
    end_verbatim
}

function testRegister()
{
    local exitCode 

    print_title "The test to register a cluster is starting."
    begin_verbatim

    #
    # Registering the cluester that we just created and dropped.
    #
    mys9s cluster \
        --register \
        --cluster-type=group_replication \
        --nodes=$NODES \
        --vendor=percona \
        --db-admin-passwd="$MYSQL_ROOT_PASSWORD" \
        --cluster-name=my_cluster_$$ \
        $LOG_OPTION

    check_exit_code $?
    
    s9s cluster --list --long
    s9s node --list --long
    s9s node --list --node-format="%12R %N\n"
    end_verbatim
}

#
# Running the requested tests.
#
startTests
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testDrop
    runFunctionalTest testRegister
fi

if [ "$FAILED" == "no" ]; then
    print_title "The test script is now finished. No errors were found."
else
    print_title "The test script is now finished. Some failures were detected."
fi

endTests


