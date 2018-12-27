#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
MYDIR=$(readlink -m $MYDIR)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
LAST_CONTAINER_NAME=""
COMMAND_LINE_OPTIONS="$0 $*"

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
 --install        Install the server and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o createServer     Creates a new cmon-cloud container server. 
  o registerServer   Unregisters and then registers the previous server. 
  o createContainer  Creates a new container.
  o createFail       A container creation that should fail.
  o createCluster    Creates a cluster on VMs created on the fly.
  o dropCluster      Drops the previously created cluster.
  o deleteContainer  Deleting the containers created by the test.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,install,\
reset-config,server:" \
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
# This will install a new cmon-cloud server. 
#
function createServer()
{
    local node001="ft_containers_az_01_$$"
    local nodeName

    print_title "Creating Container Server"
    
    echo "Creating node #0"
    nodeName=$(create_node --autodestroy "$node001")

    #
    # Creating a container.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName" \
        $LOG_OPTION

    check_exit_code_no_job $?

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonCloudServer \
        --server-name  "$nodeName" \
        --cloud        "az"

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
}

#
# Unregisters a server and registers it again.
#
function registerServer()
{
    local class

    #
    # Unregistering the server.
    #
    print_title "Unregistering Container Server"

    mys9s server \
        --unregister \
        --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER" \
        $LOG_OPTION

    check_exit_code_no_job $?


    #
    # Unregistering the server.
    #
    print_title "Registering Container Server"

    mys9s server \
        --register \
        --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER" \
        $LOG_OPTION

    check_exit_code_no_job $?

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonCloudServer \
        --server-name  "$CMON_CLOUD_CONTAINER_SERVER" \
        --cloud        "az"
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ftcontainersaz00$$"
    local template

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --cloud=az \
        --template="Standard_A2" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
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
        exit 1
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

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME="$container_name"
}

#
# This will try to create some containers with values that should cause failures
# (like duplicate names).
#
function createFail()
{
    local exitCode

    if [ -z "$LAST_CONTAINER_NAME" ]; then
        return 0
    fi

    #
    # Creating a container.
    #
    print_title "Creating Container with Duplicate Name"
    mys9s container \
        --create \
        --cloud=az \
        --template="Standard_A2" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "$LAST_CONTAINER_NAME"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with duplicate name should have failed."
        exit 1
    fi
    
    #
    # Creating a container with invalid provider.
    #
    print_title "Creating Container with Invalid Provider"
    mys9s container \
        --create \
        --cloud="no_such_cloud" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_az"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid cloud should have failed."
        exit 1
    fi
    
    #
    # Creating a container with invalid subnet.
    #
    print_title "Creating Container with Invalid Provider"
    mys9s container \
        --create \
        --cloud=az \
        --template="Standard_A2" \
        --subnet-id="no_such_subnet" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_az"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid subnet should have failed."
        exit 1
    fi

    #
    # Creating a container with invalid image.
    #
    print_title "Creating Container with Invalid Provider"
    mys9s container \
        --create \
        --cloud=az \
        --image="no_such_image" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_az"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid image should have failed."
        exit 1
    fi

    #
    # Deleting the container we already created because we have very limited
    # resources.
    #
    print_title "Deleting Container"

    mys9s container --delete $LOG_OPTION "$LAST_CONTAINER_NAME"
    exitCode=$?

    if [ "$exitCode" -ne 0 ]; then
        mys9s container --list --long
    fi

    check_exit_code $?

    LAST_CONTAINER_NAME=""
}

function createCluster()
{
    local node001="ftcontainersaz01$$"
    local node002="ftcontainersaz02$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster"
    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --nodes="$node001" \
        --cloud=az \
        --template="Standard_A2" \
        --containers="$node001" \
        $LOG_OPTION

    check_exit_code $?

    while true; do 
        CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
        
        if [ "$CLUSTER_ID" != 'NOT-FOUND' ]; then
            break;
        fi

        echo "Cluster '$CLUSTER_NAME' not found."
        s9s cluster --list --long
        sleep 5
    done

    if [ "$CLUSTER_ID" -gt 0 2>/dev/null ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    #
    # Adding a proxysql node.
    #
    print_title "Adding a ProxySql Node"

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="proxysql://$node002" \
        --cloud=az \
        --containers="$node002" \
        $LOG_OPTION

    check_exit_code $?
}

#
# Dropping the cluster from the controller.
#
function dropCluster()
{
    local exitCode

    print_title "Dropping the Cluster"

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

function deleteContainer()
{
    local containers
    local container

    containers="$LAST_CONTAINER_NAME"
    containers+=" ftcontainersaz01$$"
    containers+=" ftcontainersaz02$$"

    print_title "Deleting Containers"

    #
    # Deleting all the containers we created.
    #
    for container in $containers; do
        mys9s container \
            --delete \
            $LOG_OPTION \
            "$container"
    
        check_exit_code $?
    done

    s9s job --list
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    if [ "$1" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest createServer
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createServer
    runFunctionalTest registerServer
    runFunctionalTest createContainer
    runFunctionalTest createFail
    runFunctionalTest createCluster
    runFunctionalTest dropCluster
    runFunctionalTest deleteContainer
fi

endTests
