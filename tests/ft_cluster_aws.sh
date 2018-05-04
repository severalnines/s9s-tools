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

function createUser()
{
    local config_dir="$HOME/.s9s"

    print_title "Creating a User"

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
        exit 0
    fi
    
    if [ ! -f "$config_dir/sisko.pub" ]; then
        failure "Public key file 'sisko.pub' was not found."
        exit 0
    fi
}

#
# This will install a new cmon-cloud server. 
#
function createServer()
{
    local containerName="MYBASENAME_00_$$"
    local class
    local nodeName

    print_title "Creating Container Server"
    
    echo "Creating node #0"
    nodeName=$(create_node --autodestroy $containerName)

    #
    # Creating a container.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName" \
        --log

    check_exit_code_no_job $?

    while s9s server --list --long | grep refused; do
        echo "Server is refusing connections."
        mys9s server --list --long
        sleep 10
    done

    mys9s server --list --long
    check_exit_code_no_job $?
 
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$nodeName/.runtime/state

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
}

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
        --os-user=sisko \
        --os-key-file="$config_dir/sisko.key"
        --log \
        "$container_name"
    
    check_exit_code $?
    
    mys9s container --list --long

    #
    # Checking the ip and the owner.
    #
    CONTAINER_IP=$(\
        s9s server \
            --list-containers \
            --batch \
            --long  \
            "$container_name" \
        | awk '{print $7}')
    
    if [ -z "$CONTAINER_IP" -o "$CONTAINER_IP" == "-" ]; then
        failure "The container was not created or got no IP."
        s9s container --list --long
    fi
 
    #
    # Checking if the user can actually log in through ssh.
    #
#    print_title "Checking SSH Access"
#    if ! is_server_running_ssh "$CONTAINER_IP" "$owner"; then
#        failure "User $owner can not log in to $CONTAINER_IP"
#        exit 1
#    else
#        echo "SSH access granted for user '$USER' on $CONTAINER_IP."
#    fi

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME=$container_name
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
fi

endTests
