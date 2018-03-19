#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CONTAINER_SERVER=""
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
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o registerServer   Registers a new container server. No software installed.
  o createContainer  Creates a new container.
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
        -l "help,verbose,print-json,log,print-commands,reset-config,server:" \
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

if [ -z "$S9S" ]; then
    printError "The s9s program is not installed."
    exit 7
fi

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
        exit 1
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state
}

function registerServerCmonCloud()
{
    print_title "Registering Container Server"

    mys9s server \
        --register \
        --servers="cmon-cloud://10.10.1.1"
}


#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ft_containers_00_$$"
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
    
    mys9s container --list --long

    #
    # Checking the owner.
    #
    CONTAINER_IP=$(\
        s9s server \
            --list-containers \
            --batch \
            --long  "ft_containers_00_$$" \
        | awk '{print $7}')
    
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
        failure "The owner is '$owner', should be '$USER'"
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
function createContainers()
{
    local container_name1="ft_containers_10_$$"
    local container_name2="ft_containers_11_$$"
    local container_name3="ft_containers_12_$$"
    local container_name4="ft_containers_13_$$"
    local owner
    local template
    local container_ip

    #
    # Creating a container Centos 7.
    #
    print_title "Creating Centos 7 Container"
    mys9s container \
        --create \
        --image=centos_7
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name1"
    
    check_exit_code $?
    check_container "$container_name1"

    #
    # Creating a container Centos 6.
    #
    print_title "Creating Centos 6 Container"
    mys9s container \
        --create \
        --image=centos_6
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name2"
    
    check_exit_code $?
    check_container "$container_name2"
    
    #
    # Creating a container Debian Wheezy.
    #
    print_title "Creating Debian Wheezy Container"
    mys9s container \
        --create \
        --image=debian_wheezy
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name3"
    
    check_exit_code $?
    check_container "$container_name3"
    
    #
    # Creating a container Debian Wheezy.
    #
    print_title "Creating Debian Stretch Container"
    mys9s container \
        --create \
        --image=debian_stretch
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "$container_name4"
    
    check_exit_code $?
    check_container "$container_name4"

    #
    # Deleting the containers we just created.
    #
    print_title "Deleting Containers"
    mys9s container --delete --wait "$container_name1"
    check_exit_code $?
    
    mys9s container --delete --wait "$container_name2"
    check_exit_code $?
    
    mys9s container --delete --wait "$container_name3"
    check_exit_code $?
    
    mys9s container --delete --wait "$container_name4"
    check_exit_code $?
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

function createCluster()
{
    local node001="ft_containers_01_$$"
    local node002="ft_containers_02_$$"
    local node003="ft_containers_03_$$"

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

    #
    #
    #
    print_title "Adding a ProxySql Node"

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="proxysql://$node003" \
        --containers="$node003" \
        $LOG_OPTION

    check_exit_code $?
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

    print_title "Creating a Server"

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
}

#function createServerCloud()
#{
#    print_title "Creating cmon-cloud Server"
#
#    mys9s server \
#        --create \
#        --servers=cmon-cloud://$CONTAINER_IP \
#        --log
#
#    mys9s server --list --long
#
#    mys9s container \
#        --create \
#        --template=ubuntu16.04 \
#        --log \
#        vhost1
#}

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
# This will delete the container we created before.
#
function deleteContainer()
{
    print_title "Deleting Container"

    mys9s container \
        --delete \
        $LOG_OPTION \
        "ft_containers_00_$$"
    
    check_exit_code $?
    
    #mys9s container --list --long
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
    runFunctionalTest registerServer
    runFunctionalTest createContainer
    runFunctionalTest createContainers
    runFunctionalTest restartContainer
    runFunctionalTest createCluster
    runFunctionalTest createServer
    #runFunctionalTest createServerCloud
    runFunctionalTest failOnContainers
    runFunctionalTest deleteContainer
fi

endTests
