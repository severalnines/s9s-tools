#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

# The IP of the node we added first and last. Empty if we did not.
OPTION_VENDOR="percona"
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""
TESTED_CONTAINER_SERVER=""

cd $MYDIR
source include.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check container servers sharing hosts.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=STRING The SQL server provider version.

SUPPORTED TESTS:
  o createCluster    Creates a new cluster.
  o createServer     Creates a cmon-cloud server on one of the nodes.
  o createContainer  Creates a container.
  o recreateServer   Unregisters and re-registers the server.
  o deleteContainer  Deletes the container of the cmon-cloud.

EXAMPLE
 ./$MYNAME --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,vendor:" \
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
        
        --vendor)
            shift
            OPTION_VENDOR="$1"
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
function createCluster()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Testing the creation of a Galera cluster"
    begin_verbatim

    echo "Creating node #0"
    nodeName=$(create_node --autodestroy "ft_multiserver_$$_node001")
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
 
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    exitCode=$?
    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    end_verbatim
}

function createServer()
{
    local class
    local nodeName="$FIRST_ADDED_NODE"

    print_title "Creating a Container Server"
    cat <<EOF | paragraph
  This test creates a container server on the same IP that is already holding
  a galera node. In order to distinguish between these two host objects the
  controller needs the container server to have a port set in the request.
EOF

    begin_verbatim

    #
    # Creating a server.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName:9518" \
        $LOG_OPTION

    check_exit_code $?

    mys9s server --list --long
    check_exit_code_no_job $?

    mys9s container --list --long

    #
    # 
    #
    mys9s tree \
        --cmon-user=system \
        --password=secret \
        --cat .runtime/server_manager 

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$nodeName" \
        | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonCloudServer" ]; then
        failure "Created server has a '$class' class and not 'CmonCloudServer'."
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$nodeName/.runtime/state

    TESTED_CONTAINER_SERVER="$nodeName"

    end_verbatim
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ft_multiserver_00_$$"
    local template

    print_title "Creating a Container"
    begin_verbatim

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --servers=$TESTED_CONTAINER_SERVER \
        --volumes="vol1:10:hdd" \
        $LOG_OPTION \
        "$container_name"
    
    check_exit_code $?
    
    mys9s container --list --long

    #
    # Checking the ip and the owner.
    #
    CONTAINER_IP=$(get_container_ip "$container_name")
    if [ -z "$CONTAINER_IP" ]; then
        failure "The container was not created or got no IP."
        s9s container --list --long
        exit 1
    fi

    if [ "$CONTAINER_IP" == "-" ]; then
        failure "The container got no IP."
        s9s container --list --long
        exit 1
    fi


    owner=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $4}')

    if [ "$owner" != "$USER" ]; then
        failure "The owner of '$container_name' is '$owner', should be '$USER'"
        mys9s container --list --long --batch "$container_name"
    fi
   
    #
    # Checking if the user can actually log in through ssh.
    #
    print_title "Checking SSH Access"

    if ! is_server_running_ssh "$CONTAINER_IP" "$owner"; then
        failure "User $owner can not log in to $CONTAINER_IP"
        exit 1
    else
        echo "SSH access granted for user '$USER' on $CONTAINER_IP."
    fi

    #mys9s container --list --print-json

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME=$container_name

    end_verbatim
}

function recreateServer()
{
    local serverName

    #
    # First we unregister the server that will leave the cmon-cloud software
    # installed.
    #
    print_title "Unregistering and Re-registering Server"

    begin_verbatim
    serverName="$FIRST_ADDED_NODE"

    mys9s server \
        --unregister \
        --servers="cmon-cloud://$serverName"
    
    check_exit_code_no_job $?

    #
    # Then creating the server again.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$serverName:9518" \
        --log
    
    check_exit_code $?

    mys9s server --list --long
    end_verbatim
}

#
# This will delete the containers we created before.
#
function deleteContainer()
{
    local containers
    local container

    containers="$LAST_CONTAINER_NAME"

    print_title "Deleting Containers"
    begin_verbatim
    mys9s container --list --long

    #
    # Deleting all the containers we created.
    #
    for container in $containers; do
        mys9s container \
            --cmon-user=system \
            --password=secret \
            --delete \
            $LOG_OPTION \
            "$container"
    
        check_exit_code $?
    done

    mys9s job --list
    mys9s container --list --long
    end_verbatim
}


#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest createCluster
    runFunctionalTest createServer
    runFunctionalTest createContainer
    runFunctionalTest recreateServer
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createCluster
    runFunctionalTest createServer
    runFunctionalTest createContainer
    runFunctionalTest recreateServer
    runFunctionalTest deleteContainer
fi

endTests

