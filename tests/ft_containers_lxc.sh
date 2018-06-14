#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
MYHOSTNAME="$(hostname)"
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

CONTAINER_SERVER="$MYHOSTNAME"
CONTAINER_IP=""

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
 --install        Just install the server and the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o registerServer   Registers a new container server. No software installed.
  o checkServer      Checking the previously registered server.
  o createContainer  Creates a new container.
  o createAsSystem   Create a container as system user.
  o createFail       A container creation that should fail.
  o createContainers Creates several new containers with various images.
  o restartContainer Stop then start the container.
  o createCluster    Creates a new cluster on containers on the fly.
  o createServer     Creates a server from the previously created container.
  o failOnContainers Testing on unexisting containers.
  o deleteContainer  Deletes the previously created container.

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

    mys9s server --list --long
    check_exit_code_no_job $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$CONTAINER_SERVER" \
        | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonLxcServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
        s9s server --stat "$CONTAINER_SERVER"
        exit 1
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state
}

#
# Checking the previously registered container server.
#
function checkServer()
{
    local old_ifs="$IFS"
    local myhostname="$CONTAINER_SERVER"
    local line
    local cloud
    local region
    local server
    local cidr
    local vpc 
    local id

    #
    #
    #
    print_title "Checking Subnets"
    mys9s server --list-subnets --long

    IFS=$'\n'
    for line in $(s9s server --list-subnets --long --batch); do
        cloud=$(echo "$line" | awk '{print $1}')
        region=$(echo "$line" | awk '{print $2}')
        hostname=$(echo "$line" | awk '{print $3}')
        cidr=$(echo "$line" | awk '{print $4}')
        vpc=$(echo "$line" | awk '{print $5}')
        id=$(echo "$line" | awk '{print $6}')

        if [ "$cloud" != "lxc" ]; then
            failure "The cloud is '$cloud' instead of 'lxc'"
        fi
        
        if [ "$region" != "region1" ]; then
            failure "The region is '$region' instead of 'region1'"
        fi

        if [ "$hostname" != "$myhostname" ]; then
            failure "The hostname is '$hostname' instead of '$myhostname'."
        fi

# FIXME: This is very different on every test server... :(
#        if [ "$vpc" == "vpc-region1" ]; then
#            if [ "$id" != "${hostname}-br0" ]; then
#                failure "Public id is '$id' instead of '${hostname}-br0'"
#            fi
#        else
#            if [ "$id" != "${hostname}-lxcbr0" ]; then
#                failure "Private id is '$id' instead of '${hostname}-lxcbr0'"
#            fi
#        fi

        #echo "line: $line"
    done
    IFS="$old_ifs"

    #
    #
    #
    print_title "Check templates"
    mys9s server --list-templates --long
    
    IFS=$'\n'
    for line in $(s9s server --list-templates --long --batch); do
        cloud=$(echo "$line" | awk '{print $1}')
        region=$(echo "$line" | awk '{print $2}')
        hostname=$(echo "$line" | awk '{print $3}')

        if [ "$cloud" != "lxc" ]; then
            failure "The cloud is '$cloud' instead of 'lxc'"
        fi

        if [ "$region" != "region1" ]; then
            failure "The region is '$region' instead of 'region1'"
        fi
        
        if [ "$hostname" != "$myhostname" ]; then
            failure "The hostname is '$hostname' instead of '$myhostname'."
        fi
    done
    IFS="$old_ifs"
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ft_containers_lxc_00_$$"
    local template

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --template=ubuntu \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name"
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
    # Checking the template.
    #
    template=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $3}')

    if [ "$template" != "ubuntu" ]; then
        failure "The template is '$template', should be 'ubuntu'"
        exit 1
    fi

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME=$container_name
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createAsSystem()
{
    local owner
    local container_name="ft_containers_lxc_01_$$"
    local template

    print_title "Creating Container as System User"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --cmon-user=system \
        --password=secret \
        --os-user="$USER" \
        --os-key-file="/home/$USER/.ssh/id_rsa.pub" \
        --template=ubuntu \
        $LOG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name"
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

    if [ "$owner" != "system" ]; then
        failure "The owner of '$container_name' is '$owner', should be 'system'"
        exit 1
    fi
   
    #
    # Checking if the user can actually log in through ssh.
    #
    print_title "Checking SSH Access"
    if ! is_server_running_ssh "$CONTAINER_IP" "$USER"; then
        failure "User $USER can not log in to $CONTAINER_IP"
        exit 1
    else
        echo "SSH access granted for user '$USER' on $CONTAINER_IP."
    fi

    #
    # Checking the template.
    #
    template=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $3}')

    if [ "$template" != "ubuntu" ]; then
        failure "The template is '$template', should be 'ubuntu'"
        exit 1
    fi
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
        $LOG_OPTION \
        "node100"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid cloud should have failed."
        exit 1
    fi
    
    #
    # Creating a container with invalid subnet.
    #
    print_title "Creating Container with Invalid Subnet"
    mys9s container \
        --create \
        --subnet-id="no_such_subnet" \
        $LOG_OPTION \
        "node101"
    
    exitCode=$?
    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid subnet should have failed."
        exit 1
    fi

    # FIXME: well, this invalid subnet issue is only recognized after the
    # container was created.
    #mys9s container --delete $LOG_OPTION "node101"

    #
    # Creating a container with invalid image.
    #
    print_title "Creating Container with Invalid Image"
    mys9s container \
        --create \
        --image="no_such_image" \
        $LOG_OPTION \
        "node102"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid image should have failed."
        exit 1
    fi
}
#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainers()
{
    local container_name1="ft_containers_lxc_10_$$"
    local container_name2="ft_containers_lxc_11_$$"
    local container_name3="ft_containers_lxc_12_$$"
    local container_name4="ft_containers_lxc_13_$$"
    local owner
    local template
    local container_ip

    #
    # Creating a container Centos 7.
    #
    print_title "Creating Centos 7 Container"
    mys9s container \
        --create \
        --image=centos_7 \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name1"
    
    check_exit_code $?
    remember_cmon_container "$container_name1"
    check_container "$container_name1"

    #
    # Creating a container Centos 6.
    #
    print_title "Creating Centos 6 Container"
    mys9s container \
        --create \
        --image=centos_6 \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name2"
    
    check_exit_code $?
    remember_cmon_container "$container_name2"
    check_container "$container_name2"
    
    #
    # Creating a container Debian Wheezy.
    #
#    print_title "Creating Debian Wheezy Container"
#    mys9s container \
#        --create \
#        --image=debian_wheezy \
#        --servers=$CONTAINER_SERVER \
#        $LOG_OPTION \
#        "$container_name3"
#    
#    check_exit_code $?
#    remember_cmon_container "$container_name3"
#    check_container "$container_name3"
    
    #
    # Creating a container Debian Stretch.
    #
    print_title "Creating Debian Stretch Container"
    mys9s container \
        --create \
        --image=debian_stretch \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name4"
    
    check_exit_code $?
    remember_cmon_container "$container_name4"
    check_container "$container_name4"
}

#
# This test will call --stop and then --start on a previously created container.
#
function restartContainer()
{
    local container_name="$LAST_CONTAINER_NAME"
    local status_field

    if [ -z "$container_name" ]; then
        return 0
    fi

    #
    # Stopping the previously created continer. 
    #
    print_title "Stopping Container"

    mys9s container --stop $LOG_OPTION "$container_name"
    check_exit_code $?

    status_field=$(\
        s9s container --list --long --batch "$container_name" \
        | awk '{print $1}')

    if [ "$status_field" != "s" ]; then
        failure "Status is '$status_field' instead of '-'."
        exit 1
    fi

    #
    # Starting the container we just stopped.
    #
    print_title "Starting Container"

    mys9s container --start $LOG_OPTION "$container_name"
    check_exit_code $?
    
    status_field=$(\
        s9s container --list --long --batch "$container_name" \
        | awk '{print $1}')
    
    if [ "$status_field" != "u" ]; then
        failure "Status is '$status_field' instead of 'u'."
        exit 1
    fi
}

#
# This test will attempt to create a new server (install software and
# everything). For practical reasons we try to do this on the container we just
# created, this way wew are not installing software on a real server.
#
function createServer()
{
    local class

    if [ -z "$CONTAINER_IP" ]; then
        return 0
    fi

    print_title "Creating a Server on a Container"

    #
    # Creating a new server, installing software and everything.
    #
    mys9s server \
        --create \
        --servers="lxc://$CONTAINER_IP" \
        $LOG_OPTION 

    check_exit_code $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$CONTAINER_IP" | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonLxcServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
        exit 1
    fi

    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_IP/.runtime/state

    #
    # Unregistering.
    #
    print_title "Unregistering Server"
    mys9s server --unregister --servers="lxc://$CONTAINER_IP"

    check_exit_code_no_job $?
}

#
# This will try to manipulate a container that does not exist. The jobs should
# fail, the return value should be false.
#
function failOnContainers()
{
    local retcode

    print_title "Trying to Manipulate Unexisting Containers"

    #
    #
    #
    mys9s container \
        --delete \
        $LOG_OPTION \
        "unexisting_container_$$"

    retcode=$?
    if [ $retcode -eq 0 ]; then
        failure "Reporting success while deleting unexsiting container."
        exit 1
    fi
    
    #
    #
    #
    mys9s container \
        --stop \
        $LOG_OPTION \
        "unexisting_container_$$"

    retcode=$?
    if [ $retcode -eq 0 ]; then
        failure "Reporting success while stopping unexsiting container."
        exit 1
    fi
    
    #
    #
    #
    mys9s container \
        --start \
        $LOG_OPTION \
        "unexisting_container_$$"

    retcode=$?
    if [ $retcode -eq 0 ]; then
        failure "Reporting success while starting unexsiting container."
        exit 1
    fi
}

#
# This will create a cluster on containers and add a node in the next job
# creating a new container.
#
function createCluster()
{
    local node001="ft_containers_lxc_21_$$"
    local node002="ft_containers_lxc_22_$$"
    local node_ip
    local container_id

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
    remember_cmon_container "$node001"

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
    remember_cmon_container "$node002"
    
    #
    # Checking the proxysql node.
    #
    print_title "Checking the ProxySql Node"

    node_ip=$(proxysql_node_name)
    container_id=$(node_container_id "$node_ip")

    echo "       node_ip: $node_ip"
    echo "  container_id: $container_id"
    if [ -z "$node_ip" ]; then
        failure "The ProxySql node name was not found."
    fi

    if [ -z "$container_id" ]; then
        failure "The ProxySql container ID was not found."
    fi

    if [ "$container_id" == "-" ]; then
        failure "The ProxySql container ID is '-'."
    fi

    if ! is_server_running_ssh "$node_ip" "$USER"; then
        failure "Could not SSH into the ProxySql node."
    fi

    return 0
}

#
# This will delete the containers we created before.
#
function deleteContainer()
{
    local containers=$(cmon_container_list)
    local container

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

    #mys9s job --list
}


#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest registerServer
    runFunctionalTest checkServer
    runFunctionalTest createCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest checkServer

    runFunctionalTest createContainer
    runFunctionalTest createAsSystem
    runFunctionalTest createFail
    runFunctionalTest createContainers
    runFunctionalTest restartContainer
    runFunctionalTest createServer
    runFunctionalTest failOnContainers
    runFunctionalTest createCluster
    runFunctionalTest deleteContainer
fi

endTests
