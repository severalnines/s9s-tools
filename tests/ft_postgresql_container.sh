#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"
LOG_OPTION="--wait"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"

cd $MYDIR
source ./include.sh
source ./include_lxc.sh
source ./include_user.sh

PROVIDER_VERSION=$POSTGRESQL_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]
 Test script for s9s to check various error conditions.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the server and the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o createUserSisko  Creates a user to work with.
  o registerServer   Registers a new container server. No software installed.
  o createCluster    Creates a cluster.
  o removeCluster    Drops the cluster, removes containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,install,reset-config,\
server:" \
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

        --install)
            shift
            OPTION_INSTALL="--install"
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

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

if [ -z "$CONTAINER_SERVER" ]; then
    printError "No container server specified."
    printError "Use the --server command line option to set the server."
    exit 6
fi

#
# Creates a postgresql cluster on two newly created containers.
#
function createCluster()
{
    local config_dir="$HOME/.s9s"
    local container_name1="${MYBASENAME}_11_$$"
    local container_name2="${MYBASENAME}_12_$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=postgresql \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version=$PROVIDER_VERSION \
        --cloud=lxc \
        --nodes="$container_name1;$container_name2" \
        --containers="$container_name1;$container_name2" \
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key" \
        $LOG_OPTION 

    check_exit_code $?
    check_container_ids --postgresql-nodes
    end_verbatim

    #
    #
    #
    print_title "Waiting and Printing Lists"
    begin_verbatim

    mysleep 10
    mys9s cluster   --list --long
    mys9s node      --list --long
    mys9s container --list --long
    mys9s node      --stat
    end_verbatim
}

function removeCluster()
{
    local container_name1="${MYBASENAME}_11_$$"
    local container_name2="${MYBASENAME}_12_$$"

    #
    # Dropping and deleting.
    #
    print_title "Dropping Cluster"
    begin_verbatim

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)

    mys9s cluster \
        --drop \
        --cluster-id="$CLUSTER_ID" \
        $LOG_OPTION
    
    #check_exit_code $?

    #
    # Deleting containers.
    #
    print_title "Deleting Containers"
    
    mys9s container --delete $LOG_OPTION "$container_name1"
    check_exit_code $?
    
    mys9s container --delete $LOG_OPTION "$container_name2"
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
    runFunctionalTest createUserSisko
    runFunctionalTest registerServer
    runFunctionalTest createCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createUserSisko
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest removeCluster
fi

endTests
