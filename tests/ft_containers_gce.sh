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
    local node001="ft_containers_gce_01_$$"
    local nodeName

    print_title "Creating Container Server"
    cat <<EOF | paragraph
  This test will create a new container and install the cmon-cloud software on
  it. Then the newly created cloud server is checked.
EOF

    begin_verbatim

    echo "Creating node #0"
    nodeName=$(create_node --autodestroy "$node001")

    #
    # Creating a container.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName" \
        $LOG_OPTION

    check_exit_code $?

    mys9s server --list --long
    check_exit_code_no_job $?
    end_verbatim

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonCloudServer \
        --server-name  "$nodeName"     \
        --cloud        "gce"

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
}

#
# Unregisters a server and registers it again.
#
function testUnregister()
{
    local class
    local lines

    #
    # Unregistering the server.
    #
    print_title "Unregistering Container Server"
    cat <<EOF | paragraph
  Unregistering the container server we just created so that we can check if
  it can be registered again.
EOF
    begin_verbatim

    mys9s server \
        --unregister \
        --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER"

    check_exit_code_no_job $?
    end_verbatim
}

function testRegister()
{
    #
    # Registering the server.
    #
    print_title "Registering Container Server"
    cat <<EOF | paragraph
  Registering the server we just unregistered. Everything is there, we just need
  to register, not create.
EOF
    begin_verbatim

    mys9s server \
        --register \
        --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER" 

    check_exit_code_no_job $?

    end_verbatim

    #
    # Checking the state... 
    #
    check_container_server \
        --class        CmonCloudServer \
        --server-name  "$CMON_CLOUD_CONTAINER_SERVER"     \
        --cloud        "gce"
    end_verbatim
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#                  cmoncloudserver.cpp:1357 : MESSAGE 192.168.0.231: googleapi: Error 400: Invalid value for field 'resource.machineType': 'zones/europe-west2-b/machineTypes/a2-highgpu-1g'. Machine type with name 'a2-highgpu-1g' does not exist in zone 'europe-west2-b'., invalid.
#                 cmoncloudserver.cpp:1357 : MESSAGE 192.168.0.231: Removing instances.
#
function createContainer()
{
    local owner
    local container_name="ft-containers-gce-00-$$"
    local template
    local region

    print_title "Creating a Container on GCE"
    begin_verbatim

    mys9s server --list-regions --long --cloud=gce
    mys9s server --list-templates --long --cloud=gce

    region="europe-west2-b"
    template="e2-standard-2"

    #
    # Creating a container on GCE.
    #   a) Only one volume is supported for gce.
    #   b) Disk size can not be smaller than image size.
    #
    mys9s container \
        --create \
        --volumes="vol1:15:hdd" \
        --cloud=gce \
        --region="$region" \
        --template="$template" \
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
    else
        success "  o The container was created, OK."
    fi

    if [ "$CONTAINER_IP" == "-" ]; then
        failure "The container got no IP."
        s9s container --list --long
    else
        success "  o The container got IP, OK."
    fi

    owner=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $4}')

    if [ "$owner" != "$USER" ]; then
        failure "The owner of '$container_name' is '$owner', should be '$USER'"
    else
        success "  o The owner of '$container_name' is '$owner', OK."
    fi
   
    #
    # Checking if the user can actually log in through ssh.
    #
    print_title "Checking SSH Access"

    if ! is_server_running_ssh "$CONTAINER_IP" "$owner"; then
        failure "User $owner can not log in to $CONTAINER_IP"
    else
        success "  o SSH access granted for user '$USER' on $CONTAINER_IP."
    fi

    #mys9s container --list --print-json

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME=$container_name
    end_verbatim
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
    begin_verbatim

    mys9s container \
        --create \
        --cloud=gce \
        $LOG_OPTION \
        "$LAST_CONTAINER_NAME"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with duplicate name should have failed."
    else
        success "  o Creating container with duplicate name failed, OK."
    fi

    end_verbatim
    
    #
    # Creating a container with invalid provider.
    #
    print_title "Creating Container with Invalid Provider"
    begin_verbatim

    mys9s container \
        --create \
        --cloud="no_such_cloud" \
        $LOG_OPTION \
        "ft-containers-gce"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid cloud should have failed."
    else
        success "  o Creating container with invalid cloud failed, OK."
    fi

    end_verbatim
    
    #
    # Creating a container with invalid subnet.
    #
    print_title "Creating Container with Invalid Provider"
    begin_verbatim
    mys9s container \
        --create \
        --subnet-id="no_such_subnet" \
        --cloud=gce \
        $LOG_OPTION \
        "ft-containers-gce"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid subnet should have failed."
    else
        success "  o Creating container with invalid subnet failed, OK."
    fi

    end_verbatim

    #
    # Creating a container with invalid image.
    #
    print_title "Creating Container with Invalid Provider"
    begin_verbatim
    mys9s container \
        --create \
        --image="no_such_image" \
        --cloud=gce \
        $LOG_OPTION \
        "ft-containers-gce"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid image should have failed."
    else
        success "  o Creating container with invalid image failed, OK."
    fi

    end_verbatim
}

function createCluster()
{
    local node001="ft-containers-gce-01-$$"
    local node002="ft-containers-gce-02-$$"
    local exitCode
    local template
    local region

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster"

    begin_verbatim
    region="europe-west2-b"
    template="e2-standard-2"

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.7" \
        --vendor=percona \
        --nodes="$node001" \
        --containers="$node001" \
        --cloud=gce \
        --region="$region" \
        --template="$template" \
        $LOG_OPTION

    exitCode=$?
    check_exit_code $exitCode
    if [ $exitCode -ne 0 ]; then
        end_verbatim
        return 1
    fi

    counter=0
    while true; do 
        CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
        
        if [ "$CLUSTER_ID" != 'NOT-FOUND' ]; then
            break;
        fi
        
        if [ "$counter" -gt '2' ]; then
            break;
        fi

        echo "Cluster '$CLUSTER_NAME' not found."

        let counter+=1
        s9s cluster --list --long
        sleep 10
    done

    if [ "$CLUSTER_ID" -gt 0 2>/dev/null ]; then
        success "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    check_container_ids --galera-nodes
    end_verbatim

    #
    # Adding a proxysql node.
    #
    # We don't need to set the region here, it is inherited from the cluster, 
    # the controller will put this new container into the same region the nodes
    # are.
    #
    print_title "Adding a ProxySql Node"
    begin_verbatim

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="proxysql://$node002" \
        --containers="$node002" \
        --region="$region" \
        --template="$template" \
        --cloud=gce \
        $LOG_OPTION

    check_exit_code $?
    end_verbatim
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
    containers+=" ft-containers-gce-01-$$"
    containers+=" ft-containers-gce-02-$$"

    print_title "Deleting Containers"
    begin_verbatim

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
    end_verbatim
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
    
    runFunctionalTest testUnregister
    runFunctionalTest testRegister

    runFunctionalTest createContainer
    runFunctionalTest createFail
    runFunctionalTest createCluster
    runFunctionalTest deleteContainer
    runFunctionalTest unregisterServer
fi

endTests
