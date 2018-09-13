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
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check cluster creation on containers.

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
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "sisko"
    
    check_exit_code_no_job $?

    ls -lha $HOME/.s9s/sisko*

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
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?

    check_container_server \
        --server-name "$CONTAINER_SERVER" \
        --class       "CmonLxcServer"
}

#
# Creates then destroys a cluster on lxc.
#
function createContainer()
{
    local config_dir="$HOME/.s9s"
    local container_name="${MYBASENAME}_01_$$"
    local template
    local owner

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        --cloud=lxc \
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key" \
        $LOG_OPTION \
        "$container_name"
    
    check_exit_code $?
    
    mys9s container --list --long

    #
    # Checking the ip and the owner.
    #
    CONTAINER_IP=$(get_container_ip "$container_name")
    
    if [ -z "$CONTAINER_IP" -o "$CONTAINER_IP" == "-" ]; then
        failure "The container was not created or got no IP."
        s9s container --list --long
    fi
 
    #
    # Checking if the owner can actually log in through ssh.
    #
    print_title "Checking SSH Access for '$USER'"
    is_server_running_ssh "$CONTAINER_IP" "$USER"

    if [ $? -ne 0 ]; then
        failure "User $USER can not log in to $CONTAINER_IP"
    else
        echo "SSH access granted for user '$USER' on $CONTAINER_IP."
    fi
    
    #
    # Checking that sisko can log in.
    #
    print_title "Checking SSH Access for 'sisko'"
    is_server_running_ssh \
        --current-user "$CONTAINER_IP" "sisko" "$config_dir/sisko.key"

    if [ $? -ne 0 ]; then
        failure "User 'sisko' can not log in to $CONTAINER_IP"
    else
        echo "SSH access granted for user 'sisko' on $CONTAINER_IP."
    fi

    #
    # Deleting the container we just created.
    #
    print_title "Deleting Container"

    mys9s container --delete $LOG_OPTION "$container_name"
    check_exit_code $?
}

function createCluster()
{
    local config_dir="$HOME/.s9s"
    local container_name1="${MYBASENAME}_11_$$"
    local container_name2="${MYBASENAME}_12_$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$container_name1;$container_name2" \
        --containers="$container_name1;$container_name2" \
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key" \
        $LOG_OPTION 

    check_exit_code $?
    check_container_ids --galera-nodes

    #
    #
    #
    print_title "Waiting and Printing Lists"
    sleep 10
    mys9s cluster   --list --long
    mys9s node      --list --long
    mys9s container --list --long
    mys9s node      --stat
}

function removeCluster()
{
    local container_name1="${MYBASENAME}_11_$$"
    local container_name2="${MYBASENAME}_12_$$"

    #
    # Dropping and deleting.
    #
    print_title "Dropping Cluster"
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
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

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
