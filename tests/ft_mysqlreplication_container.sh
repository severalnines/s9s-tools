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
OPTION_VENDOR="percona"

cd $MYDIR
source ./include.sh
source ./include_lxc.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

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
  o createUser       Creates a user to work with.
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

function createUser()
{
    local config_dir="$HOME/.s9s"
    local myself

    #
    #
    #
    print_title "Creating a User"

    begin_verbatim

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Captain" \
        --first-name="Benjamin" \
        --last-name="Sisko"   \
        --email-address="sisko@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "sisko"
    
    check_exit_code_no_job $?

    end_verbatim

    #ls -lha "$config_dir"

    if [ ! -f "$config_dir/sisko.key" ]; then
        failure "Secret key file 'sisko.key' was not found."
        exit 0
    fi

    if [ ! -f "$config_dir/sisko.pub" ]; then
        failure "Public key file 'sisko.pub' was not found."
        exit 0
    fi

    myself=$(s9s user --whoami)
    if [ "$myself" != "$USER" ]; then
        failure "Whoami returns $myself instead of $USER."
    fi
}

#
# Creates a postgresql cluster on two newly created containers.
#
function createCluster()
{
    local config_dir="$HOME/.s9s"
    local container_name1="${MYBASENAME}_11_$$"
    local container_name2="${MYBASENAME}_12_$$"
    local container_name3="${MYBASENAME}_13_$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"

    begin_verbatim

    mys9s cluster \
        --create \
        --template="ubuntu" \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=mysqlreplication \
        --provider-version=$PROVIDER_VERSION \
        --vendor="$OPTION_VENDOR" \
        --cloud=lxc \
        --nodes="$container_name1;$container_name2;$container_name3" \
        --containers="$container_name1;$container_name2;$container_name3" \
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key" \
        $LOG_OPTION 

    check_exit_code $?
    check_container_ids --replication-nodes

    end_verbatim

    #
    #
    #
    print_title "Waiting and Printing Lists"
    sleep 10

    begin_verbatim

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
    local container_name3="${MYBASENAME}_13_$$"

    #
    # Dropping and deleting.
    #
    print_title "Dropping Cluster"
    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)

    begin_verbatim

    mys9s cluster \
        --drop \
        --cluster-id="$CLUSTER_ID" \
        $LOG_OPTION

    end_verbatim
    
    #check_exit_code $?

    #
    # Deleting containers.
    #
    print_title "Deleting Containers"

    begin_verbatim
    
    mys9s container --delete $LOG_OPTION "$container_name1"
    check_exit_code $?
    
    mys9s container --delete $LOG_OPTION "$container_name2"
    check_exit_code $?
    
    mys9s container --delete $LOG_OPTION "$container_name3"
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
    runFunctionalTest createUser
    runFunctionalTest registerServer
    runFunctionalTest createCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createUser
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest removeCluster
fi

endTests
