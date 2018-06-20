#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"
LOG_OPTION="--wait"

export CMON_USER_PASSWORD="siskopassword"
CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"

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
 
  $MYNAME - Test script for s9s to check the autogeneration of ssh keys.

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
  o createContainer  Creates a container.
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
    # Adding an extra user that we will use for testing.
    #
    print_title "Creating a New User"

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Captain" \
        --first-name="Benjamin" \
        --last-name="Sisko"   \
        --email-address="sisko@ds9.com" \
        --new-password="$CMON_USER_PASSWORD" \
        --group=admins \
        --batch \
        "sisko"
    
    check_exit_code_no_job $?

    myself=$(s9s user --whoami --password="$CMON_USER_PASSWORD")
    if [ "$myself" != "sisko" ]; then
        mys9s user --whoami --password="$CMON_USER_PASSWORD"
        failure "Whoami returns $myself instead of sisko."
    fi
}

#
# This will register the container server. 
#
function registerServer()
{
    local class

    print_title "Registering Container Server"

    #
    # Creating a container.
    #
    mys9s server \
        --register \
        --password="$CMON_USER_PASSWORD" \
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?

    mys9s server --list --long --password="$CMON_USER_PASSWORD"
    check_exit_code_no_job $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$CONTAINER_SERVER" --password="$CMON_USER_PASSWORD" \
        | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonLxcServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
        exit 1
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree \
        --cat \
        --password="$CMON_USER_PASSWORD" \
        /$CONTAINER_SERVER/.runtime/state
}

function createCluster()
{
    local config_dir="$HOME/.s9s"
    local container_name1="${MYBASENAME}_11_$$"
    #local container_name2="${MYBASENAME}_12_$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"

    # FIXME: If we set the username everything is ok until the Workflow
    # reports an error because we have this in the test:
    #   CmonConfiguration::setOverride(PropOsUser, CmonString(getenv("USER")));
    mys9s cluster \
        --create \
        --password="$CMON_USER_PASSWORD" \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$container_name1;$container_name2" \
        --containers="$container_name1;$container_name2" \
        --generate-key \
        --os-user=pipas \
        $LOG_OPTION 

    # FIXME: check_exit_code do not support user password.
    check_exit_code_no_job $?
    #check_container_ids --galera-nodes

    #
    #
    #
    print_title "Waiting and Printing Lists"
    sleep 10
    mys9s cluster   --list --long --password="$CMON_USER_PASSWORD"
    mys9s node      --list --long --password="$CMON_USER_PASSWORD"
    mys9s container --list --long --password="$CMON_USER_PASSWORD"
    mys9s node      --stat --password="$CMON_USER_PASSWORD"
}

function removeCluster()
{
    local container_name1="${MYBASENAME}_11_$$"
    #local container_name2="${MYBASENAME}_12_$$"

    #
    # Dropping and deleting.
    #
    print_title "Dropping the Cluster"
    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)

    echo "  CLUSTER_ID: '$CLUSTER_ID'"
    if [ "$CLUSTER_ID" == "NOT-FOUND" ]; then
        failure "Cluster not found."
    else
        mys9s cluster \
            --drop \
            --cluster-id="$CLUSTER_ID" \
            --password="$CMON_USER_PASSWORD" \
            $LOG_OPTION
        
        check_exit_code $?
    fi

    #
    # Deleting containers.
    #
    print_title "Deleting Containers"
    
    mys9s container \
        --delete \
        --password="$CMON_USER_PASSWORD" \
        $LOG_OPTION \
        "$container_name1"

    check_exit_code $?
    
    mys9s container \
        --delete \
        $LOG_OPTION \
        --password="$CMON_USER_PASSWORD" \
        "$container_name2"

    check_exit_code $?
}

#
# Running the requested tests. We are not granting the first user the usual
# ways, this test creates a user without a key.
#
startTests
reset_config
#grant_user

#s9s event --list --with-event-job &
#EVENT_HANDLER_PID=$!

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
    runFunctionalTest createContainer
    runFunctionalTest createCluster
    runFunctionalTest removeCluster
fi

#kill $EVENT_HANDLER_PID
endTests
