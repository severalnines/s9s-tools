#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
LAST_CONTAINER_NAME=""

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
 --install        Just install the server and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o createServer     Creates a new cmon-cloud container server. 
  o registerServer   Unregisters and then registers the previous server. 
  o createContainer  Creates a new container.
  o containerImages  Creates containers with various images.
  o createFail       A container creation that should fail.
  o createCluster    Creates a cluster on VMs created on the fly.
  o deleteContainer  Deletes all the containers that were created.
  o unregisterServer Unregistering cmon-cloud server.

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
# This will install a new cmon-cloud server. 
#
function createServer()
{
    local node001="ft_containers_aws_01_$$"
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
        --cloud        "aws"

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
}

#
# Unregisters a server and registers it again.
#
function registerServer()
{
    local class
    local lines

    #
    # Unregistering the server.
    #
    print_title "Unregistering Container Server"

    mys9s server \
        --unregister \
        --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER"

    check_exit_code_no_job $?

    #
    # Registering the server.
    #
    print_title "Registering Container Server"

    mys9s server \
        --register \
        --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER" 

    check_exit_code_no_job $?

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonCloudServer \
        --server-name  "$CMON_CLOUD_CONTAINER_SERVER" \
        --cloud        "aws"
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ft_containers_aws_00_$$"
    local template

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        --volumes="vol1:5:hdd;vol2:10:hdd" \
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

    #mys9s container --list --print-json

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME=$container_name
}

#
# Creating containers with various images on AWS.
#
function containerImages()
{
    local container_name
    local container_ip
    local image
    local owner
    local n=0

    print_title "Creating Containers with Images"

    for image in "ubuntu16.04" "debian8" "centos7"; do
        container_name=$(printf "ft_containers_aws_2%d_$$" "$n")

        #
        # Creating the container.
        #
        mys9s container \
            --create \
            --servers=$CMON_CLOUD_CONTAINER_SERVER \
            --cloud="aws" \
            --image="$image" \
            $LOG_OPTION \
            "$container_name"
    
        check_exit_code $?
    
        #
        # Checking the container.
        #
        container_ip=$(get_container_ip "$container_name")
        if [ -z "$container_ip" ]; then
            failure "The container was not created or got no IP."
            s9s container --list --long
            exit 1
        fi
        
        owner=$(\
            s9s container --list --long --batch "$container_name" | \
            awk '{print $4}')

        if [ "$owner" != "$USER" ]; then
            failure "The owner of '$container_name' is '$owner', not '$USER'"
            exit 1
        fi

        if ! is_server_running_ssh "$container_ip" "$owner"; then
            failure "User $owner can not log in to $container_ip"
            exit 1
        else
            echo "SSH access granted for user '$USER' on $container_ip."
        fi

        #
        # Deleting the container.
        #
        mys9s container --delete $LOG_OPTION "$container_name"
        check_exit_code $?

        let n+=1
    done
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
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "$LAST_CONTAINER_NAME"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with duplicate name should have failed."
        exit 1
    else 
        echo "Yes, this expected to fail."
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
        "ft_containers_aws"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid cloud should have failed."
        exit 1
    else 
        echo "Yes, this expected to fail."
    fi
    
    #
    # Creating a container with invalid subnet.
    #
    print_title "Creating Container with Invalid Provider"
    mys9s container \
        --create \
        --subnet-id="no_such_subnet" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_aws"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid subnet should have failed."
        exit 1
    else 
        echo "Yes, this expected to fail."
    fi

    #
    # Creating a container with invalid image.
    #
    print_title "Creating Container with Invalid Provider"
    mys9s container \
        --create \
        --image="no_such_image" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_aws"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid image should have failed."
        exit 1
    else 
        echo "Yes, this expected to fail."
    fi
    
    #
    # Creating a container with invalid firewall.
    #
    print_title "Creating Container with Invalid Firewall"
    mys9s container \
        --create \
        --firewalls="nosuchfirewall" \
        --servers=$CMON_CLOUD_CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_aws"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid firewall should have failed."
        exit 1
    else 
        echo "Yes, this expected to fail."
    fi
}

function createCluster()
{
    local node001="ft_containers_aws_01_$$"
    local node002="ft_containers_aws_02_$$"

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

    check_container_ids --galera-nodes

    #
    # Adding a proxysql node.
    #
    print_title "Adding a ProxySql Node"

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="proxysql://$node002" \
        --containers="$node002" \
        $LOG_OPTION

    check_exit_code $?
}

#
# This will delete the containers we created before.
#
function deleteContainer()
{
    local containers
    local container

    #mys9s container --list --print-json

    containers="$LAST_CONTAINER_NAME"
    containers+=" ft_containers_aws_01_$$"
    containers+=" ft_containers_aws_02_$$"

    print_title "Deleting Containers"

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

    s9s job --list
}

function unregisterServer()
{
    if [ -n "$CMON_CLOUD_CONTAINER_SERVER" ]; then
        print_title "Unregistering Cmon-cloud server"
        
        mys9s server \
            --unregister \
            --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER"

        check_exit_code_no_job $?
    fi

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
    runFunctionalTest containerImages
    runFunctionalTest createFail
    runFunctionalTest createCluster
    runFunctionalTest deleteContainer
    runFunctionalTest unregisterServer
fi

endTests
