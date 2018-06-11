#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""
DATABASE_USER="$USER"
PROVIDER_VERSION="5.7"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""

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
 
  $MYNAME - Test script for s9s to check Galera clusters.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --provider-version=STRING The SQL server provider version.

EXAMPLE
 ./ft_galera.sh --print-commands --server=storage01 --reset-config --install
EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
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

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

if [ -z $(which pip-container-create) ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode
    local nNodes=1

    print_title "Creating a Cluster"

    for ((n=0;n<nNodes;++n)); do
        echo "Creating container #${n}."
        nodeName=$(create_node --autodestroy)
        nodes+="$nodeName;"
    
        if [ "$n" == "0" ]; then
            FIRST_ADDED_NODE=$nodeName
        fi
    done
       
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
        exit 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
}

function testTree()
{
    local retcode
    local files
    local file

    print_title "Checking Access Rights"
    mys9s tree --tree --all

    if ! s9s tree --access --privileges="rwx" /; then
        failure "The user has no access to '/'"
        exit 1
    fi
    
    if ! s9s tree --access --privileges="r" /groups; then
        failure "The user has no access to '/groups'"
        exit 1
    fi
    
    if s9s tree --access --privileges="rwx" /groups; then
        failure "The user has write access to '/groups'"
        exit 1
    fi

    files+="/.runtime/cluster_manager"
    files+=" /.runtime/host_manager"
    files+=" /.runtime/job_manager"
    files+=" /.runtime/mutexes"
    files+=" /.runtime/server_manager"
    files+=" /.runtime/threads"
    files+=" /.runtime/user_manager"

    #
    # Normal user should not have access to some special files.
    #
    for file in $files; do
        s9s tree \
            --access \
            --privileges="r" \
            "$file"

        retcode=$?
        if [ "$retcode" -eq 0 ]; then
            failure "Normal user should not have access to '/.runtime' files"
            exit 1
        fi
    done

    #
    # Checking if the system user has access to some special files.
    #
    for file in $files; do
        s9s tree \
            --access \
            --privileges="r" \
            --cmon-user=system \
            --password=secret \
            "$file" 
    
        retcode=$?
        if [ "$retcode" -ne 0 ]; then
            failure "The system user should have access to '$file' file"
            exit 1
        fi
    done

    #
    #
    #
    file="/$CLUSTER_NAME/.runtime/state"
    s9s tree --cat \
        --cmon-user=system \
        --password=secret \
        "$file" \
    | \
    grep --quiet CmonHostCollector

    retcode=$?
    if [ "$retcode" -ne 0 ]; then
        failure "The file file content for '$file' is not ok."
        exit 1
    fi
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
    runFunctionalTest testTree
fi

endTests


