#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CONTAINER_SERVER=""
CONTAINER_IP=""

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
    print_title "Registering Container Server"

    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER"

    check_exit_code_no_job $?

    mys9s server --list --long
    check_exit_code_no_job $?
}

function registerServerFails()
{
    print_title "Registrations that should Fail"

    # Duplicate entries in one call.
    mys9s server \
        --register \
        --servers="lxc://core2;lxc://core2"

    if [ $? -eq 0 ]; then
        failure "Registration should have failed (duplicate entries)"
        exit 1
    fi

    # Folder that does not exist.
    mys9s server \
        --register \
        --servers="lxc://core3?cdt_path=/non-existing-folder"

    if [ $? -eq 0 ]; then
        failure "Registration should have failed (invalid folder)"
        exit 1
    fi

    # The server that we already registered
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER"

    if [ $? -eq 0 ]; then
        failure "Registration should have failed (server already registered)"
        exit 1
    fi
}

function createContainer()
{
    local owner
    local container_name="ft_containers_$$"
    local template

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
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
            --long  "ft_containers_$$" \
        | awk '{print $6}')
    
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
    # Checking the template.
    #
    template=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $3}')

#    if [ "$template" != "ubuntu" ]; then
#        failure "The template is '$template', should be 'ubuntu'"
#        exit 1
#    fi
}

function createServer()
{
    print_title "Installing Container as Server"

    if [ -z "$CONTAINER_IP" ]; then
        failure "Container IP was not found."
        exit 1
    fi

    if [ "$CONTAINER_IP" == "-" ]; then
        failure "The container has no IP."
        exit 1
    fi

    #
    # Installing the container as a container server.
    #
    mys9s server \
        --create \
        --log \
        --timeout=30 \
        --servers="lxc://$CONTAINER_IP"

    if [ $? -ne 0 ]; then
        failure "The job failed"
        exit 1
    fi

    mys9s tree --tree
    mys9s server --list --long
}

function createContainerInContainer()
{
    local owner
    local container_name="ft_containers_sub_$$"
    local template

    print_title "Creating Container"

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --servers=$CONTAINER_IP \
        $LOG_OPTION \
        "$container_name"

    check_exit_code $?

    s9s container --list --long
}

#
# This will delete the container we created before.
#
function deleteContainer()
{
    print_title "Deleting Container"

    mys9s container \
        --delete \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        "ft_containers_$$"
    
    check_exit_code $?
    
    mys9s container --list --long
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
    runFunctionalTest registerServerFails
    runFunctionalTest createContainer
    runFunctionalTest createServer
    #runFunctionalTest createContainerInContainer
    runFunctionalTest deleteContainer
fi

endTests
