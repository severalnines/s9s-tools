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

function installCmonCloud()
{
    local binaryFile="/usr/sbin/cmon-cloud"
    local doUpdate
    local DOWNLOAD_URL='http://www.severalnines.com/downloads/cmon'

    print_title "Installing cmon-cloud"
    if [ -f "$binaryFile" ]; then
        failure "File '$binaryFile' already exists."
        return 1
    fi

    if [ ! -f "/etc/apt/sources.list.d/s9s-repo-nightly.list" ]; then
        echo "deb [arch=amd64] http://repo.severalnines.com/repos-nightly/deb ubuntu main" \
            | sudo tee "/etc/apt/sources.list.d/s9s-repo-nightly.list"

        doUpdate="true"
    fi

    wget $DOWNLOAD_URL/s9s-repo.list -P /etc/apt/sources.list.d/

    wget http://repo.severalnines.com/severalnines-repos.asc
    sudo apt-key add severalnines-repos.asc
    rm -f severalnines-repos.asc
    doUpdate=true


    if [ -n "$doUpdate" ]; then
        sudo apt -y update
    fi

    echo "# sudo apt -y --force-yes install clustercontrol-cloud"
    sudo apt -y --force-yes install clustercontrol-cloud
    if [ $? -ne 0 ]; then
        failure "Failed to install 'clustercontrol-cloud'"
        return 1
    fi

    if [ ! -f "$binaryFile" ]; then
        failure "File '$binaryFile' does not exist."
        return 1
    fi

    #
    #
    #
    print_title "Starting cmon-cloud"
    echo "# sudo systemctl start cmon-cloud"
    sudo systemctl start cmon-cloud

    #sleep 5
    echo "# sudo systemctl status cmon-cloud"
    sudo systemctl status cmon-cloud

    echo "# ps axu | grep cmon-cloud"
    ps axu | grep cmon-cloud

}

function removeCmonCloud()
{
    print_title "Removing cmon-cloud"

    echo "# sudo systemctl stop cmon-cloud"
    sudo systemctl stop cmon-cloud
    sleep 5
    
    echo "# sudo systemctl status cmon-cloud"
    sudo systemctl status cmon-cloud

    echo "# ps aux | grep cmon-cloud"
    ps aux | grep cmon-cloud
    
    #echo "# killall cmon-cloud"
    #killall cmon-cloud
    
    #echo "# killall -9 cmon-cloud"
    #killall -9 cmon-cloud
    
    echo "# ps aux | grep cmon-cloud"
    ps aux | grep cmon-cloud

    echo "# sudo apt -y --force-yes remove clustercontrol-cloud"
    sudo apt -y --force-yes remove clustercontrol-cloud
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ft_autoregister_aws_$$"
    local template
    local retCode

    print_title "Creating Container"
    cat <<EOF
  This test will create a container before any container server is created or
registered manually. The controller has an installed and started cmon-cloud 
that should be automatically registered and so the creation of the container
should be successful.

EOF

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --cloud=aws \
        --log \
        "$container_name"
   
    retCode=$?
    if [ "$retCode" -ne 0 ]; then
        mys9s server    --list --long
        mys9s server    --stat
    fi

    #check_exit_code $?
    #mys9s container --list --long
    #mys9s server    --list --long

    #
    # Checking the container and now we should already have an auto-registered
    # container server.
    #
    CONTAINER_IP=$(get_container_ip "$container_name")

    check_container \
        --owner        "$USER"      \
        --cloud        "aws"        \
        --state        "RUNNING"    \
        --server       "localhost"  \
        --prot         "cmon-cloud" \
        --group        "testgroup"  \
        --acl          "rwxrw---- " \
        "$container_name"

    check_container_server \
        --class        CmonCloudServer \
        --server-name  "localhost" \
        --cloud        "aws"

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME="$container_name"
}

function deleteContainer()
{
    local containers
    local container

    containers="$LAST_CONTAINER_NAME"

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
    runFunctionalTest installCmonCloud
    runFunctionalTest createContainer
    runFunctionalTest deleteContainer
    runFunctionalTest removeCmonCloud
fi

endTests
