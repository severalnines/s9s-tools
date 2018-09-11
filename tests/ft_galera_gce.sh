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
OPTION_VENDOR="mariadb"
PROVIDER_VERSION="10.2"

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
    local class
    local nodeName

    print_title "Creating Container Server"
    
    echo "Creating node #0"
    nodeName=$(create_node --autodestroy)

    #
    # Creating a cmon-cloud server.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName" \
        $LOG_OPTION

    check_exit_code_no_job $?

    while s9s server --list --long | grep refused; do
        sleep 10
    done

    mys9s server --list --long
    check_exit_code_no_job $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$nodeName" \
        | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonCloudServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
        exit 1
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$nodeName/.runtime/state

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
    mys9s server --list-images --long
    mys9s server --list-templates --long --cloud=gce
}

function createCluster()
{
    local node001="ft-galera-gce-01-$$"
    local node002="ft-galera-gce-02-$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on GCE"
    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="$PROVIDER_VERSION" \
        --vendor=$OPTION_VENDOR \
        --nodes="$node001;$node002" \
        --containers="$node001;$node002" \
        --image="centos7" \
        --cloud=gce \
        --region="europe-west2-b" \
        --log

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
    #
    #
    print_title "Printing Lists"
    mys9s cluster   --list --long
    mys9s node      --list --long
    mys9s container --list --long
    mys9s tree --list --long --recursive --cmon-user=system --password=secret
}

#
# This will delete the containers we created before.
#
function deleteContainer()
{
    local containers
    local container

    containers+=" ft-galera-gce-01-$$"
    #containers+=" ft-galera-gce-02-$$"

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
    runFunctionalTest createServer
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createServer
    runFunctionalTest createCluster
    runFunctionalTest deleteContainer
    runFunctionalTest unregisterServer
fi

endTests
