#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

MYSQL_ROOT_PASSWORD=$(generate_strong_password)

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test register cluster on PostgreSql.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
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
    local nodeName

    print_title "Creating NDB Cluster"
    begin_verbatim

    nodeName=$(create_node --autodestroy)
    NODES+="mysql://$nodeName;ndb_mgmd://$nodeName;"

    nodeName=$(create_node --autodestroy)
    NODES+="mysql://$nodeName;ndb_mgmd://$nodeName;"
    
    nodeName=$(create_node --autodestroy)
    NODES+="ndbd://$nodeName;"
    
    nodeName=$(create_node --autodestroy)
    NODES+="ndbd://$nodeName"

    #
    # Creating an NDB cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=ndb \
        --nodes="$NODES" \
        --vendor=oracle \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin-passwd="$MYSQL_ROOT_PASSWORD" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" == "NOT-FOUND" ]; then
        failure "Cluster was not created."
        end_verbatim
        return 1
    elif [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi


    end_verbatim
}

#
# Dropping the cluster from the controller.
#
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
        $LOG_OPTION
   
    check_exit_code $?
    end_verbatim
}

function testRegister()
{
    local exitCode 

    print_title "Registering the Cluster"
    begin_verbatim

    #
    # Registering the cluester that we just created and dropped.
    #
    mys9s cluster \
        --register \
        --cluster-type=ndb \
        --nodes="$NODES" \
        --db-admin-passwd="$MYSQL_ROOT_PASSWORD" \
        --cluster-name=my_cluster_$$ \
        $LOG_OPTION

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    s9s cluster --list --long
    s9s node --list --long
    end_verbatim
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testCreateCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testDrop
    runFunctionalTest testRegister
fi

endTests

