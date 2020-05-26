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
Usage: $MYNAME [OPTION]... [TESTNAME]
 Test script for s9s to check various error conditions.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Leaves the container server when finished.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS
  o createUser       Creates a cmon user for the tests.
  o createServer     Creates a cmon-cloud server.
  o createContainer  Creates a container on cmon-cloud.
  o createCluster    Creates a cluster on some new containers.
  o deleteContainers Drops the cluster and the containers.

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

    ls -lha "$config_dir"

    if [ ! -f "$config_dir/sisko.key" ]; then
        failure "Secret key file 'sisko.key' was not found."
    else
        success "  o Secret key file 'sisko.key' was found."
    fi

    if [ ! -f "$config_dir/sisko.pub" ]; then
        failure "Public key file 'sisko.pub' was not found."
    else
        success "  o Public key file 'sisko.pub' was found."
    fi

    myself=$(s9s user --whoami)
    if [ "$myself" != "$USER" ]; then
        failure "Whoami returns $myself instead of $USER."
    fi

    end_verbatim
}

#
# This will install a new cmon-cloud server. 
#
function createServer()
{
    local containerName="${MYBASENAME}_00_$$"
    local class
    local nodeName

    print_title "Creating Container Server"
    begin_verbatim
    
    echo "Creating node #0"
    #nodeName=$(create_node --autodestroy $containerName)
    nodeName=$(create_node --autodestroy $containerName)

    #
    # Creating a container.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName" \
        $LOG_OPTION

    check_exit_code_no_job $?

    while s9s server --list --long | grep refused; do
        echo "Server is refusing connections."
        mys9s server --list --long
        sleep 10
    done

    mys9s server --list --long
    check_exit_code_no_job $?
 
    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CMON_CLOUD_CONTAINER_SERVER/.runtime/state
    end_verbatim
}

#
# Creates then destroys a cluster on AWS.
#
function createContainer()
{
    local config_dir="$HOME/.s9s"
    local container_name="${MYBASENAME}_01_$$"
    local template
    local owner

    print_title "Creating Container"
    begin_verbatim

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        --cloud=aws \
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

    end_verbatim
 
    #
    # Checking if the owner can actually log in through ssh.
    #
    print_title "Checking SSH Access for '$USER'"
    begin_verbatim

    is_server_running_ssh "$CONTAINER_IP" "$USER"

    if [ $? -ne 0 ]; then
        failure "User $USER can not log in to $CONTAINER_IP"
    else
        success "  o SSH access granted for user '$USER' on $CONTAINER_IP."
    fi
    end_verbatim
    
    #
    # Checking that sisko can log in.
    #
    print_title "Checking SSH Access for 'sisko'"
    begin_verbatim

    is_server_running_ssh \
        --current-user "$CONTAINER_IP" "sisko" "$config_dir/sisko.key"

    if [ $? -ne 0 ]; then
        failure "User 'sisko' can not log in to $CONTAINER_IP"
    else
        success "  o SSH access granted for user 'sisko' on $CONTAINER_IP."
    fi
    end_verbatim

    #
    # Deleting the container we just created.
    #
    print_title "Deleting Container"
    begin_verbatim

    mys9s container --delete $LOG_OPTION "$container_name"
    check_exit_code $?
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CMON_CLOUD_CONTAINER_SERVER/.runtime/state
    end_verbatim
}

function createCluster()
{
    local config_dir="$HOME/.s9s"
    local container_name1="${MYBASENAME}_11_$$"
    local container_name2="${MYBASENAME}_12_$$"
    local node_ip
    local container_id

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on AWS"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=aws \
        --nodes="$container_name1;$container_name2" \
        --containers="$container_name1;$container_name2" \
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key" \
        $LOG_OPTION

    check_exit_code $?
    check_container_ids --galera-nodes
    end_verbatim

    return 0
}

function deleteContainers()
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
    end_verbatim

    #
    # Deleting containers.
    #
    print_title "Deleting Containers"
    begin_verbatim
    
    mys9s container --delete $LOG_OPTION "$container_name1"
    check_exit_code $?
    
    mys9s container --delete $LOG_OPTION "$container_name2"
    check_exit_code $?

    mys9s container --list --long
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CMON_CLOUD_CONTAINER_SERVER/.runtime/state
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
    runFunctionalTest createUser
    runFunctionalTest createServer
    runFunctionalTest createContainer
    runFunctionalTest createCluster

    if [ -z "$OPTION_INSTALL" ]; then
        runFunctionalTest deleteContainers
    fi
fi

endTests
