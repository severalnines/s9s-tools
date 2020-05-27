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
source ./include_user.sh

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
 --install        Just install the server and cluster, then exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

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

#
# This will install a new cmon-cloud server. 
#
function createServer()
{
    local containerName="ft_cluster_az_00_$$"
    local class
    local nodeName

    print_title "Creating Container Server"
    
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
}

#
# Creates then destroys a cluster on Azure.
#
function createContainer()
{
    local config_dir="$HOME/.s9s"
    local container_name="ftclusteraz01$$"
    local template
    local owner

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        --cloud=az \
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
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CMON_CLOUD_CONTAINER_SERVER/.runtime/state
}

function createCluster()
{
    local config_dir="$HOME/.s9s"
    local container_name1="ftclusteraz11$$"
    local container_name2="ftclusteraz12$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on Azure"
cat <<EOF
<p>
This test seems to have a lot of issues on Azure. In fact Azure seems to have 
a lot of issues. I am trying to make the controller and s9s more robust using
Azure issues tests.
</p>
EOF
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=az \
        --nodes="$container_name1;$container_name2" \
        --containers="$container_name1;$container_name2" \
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key" \
        --log

    if ! check_exit_code --do-not-exit $?; then
        mys9s container --delete --wait "$container_name1"
        mys9s container --delete --wait "$container_name2"
        return 1
    fi

    end_verbatim

    #
    #
    #
    print_title "Waiting and Printing Lists"
    begin_verbatim

    mysleep 60
    mys9s cluster   --list --long
    mys9s node      --list --long
    mys9s container --list --long

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
echo "Test is disabled, because the cmon-cloud is unreliable."
if false; then

reset_config
grant_user

if [ -n "$OPTION_INSTALL" ]; then
    runFunctionalTest createUserSisko
    runFunctionalTest createServer
    runFunctionalTest createCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createUserSisko
    runFunctionalTest createServer
    runFunctionalTest createContainer
    runFunctionalTest createCluster
fi
fi
endTests
