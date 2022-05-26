#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait --print-request"

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
LAST_CONTAINER_NAME=""
OPTION_VENDOR="mariadb"

cd $MYDIR
source include.sh

PROVIDER_VERSION=$POSTGRESQL_DEFAULT_PROVIDER_VERSION

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
 --install        Just installs the server and the cluster and exits.
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
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:" \
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
            LOG_OPTION="--log --print-request"
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

        --)
            shift
            break
            ;;
    esac
done

#
# This will install a new cmon-cloud server. 
#
function createServer()
{
    local container_name="ft_postgresql_gce_00"
    local class
    local nodeName

    print_title "Creating Container Server"
    begin_verbatim
    
    echo "Creating node #0"
    nodeName=$(create_node --autodestroy $container_name)

    #
    # Creating a cmon-cloud server.
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
        --cloud        "gce"

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
    end_verbatim
}

function createCluster()
{
    local node001="ft-postgresql-gce-01-$$"
    local retCode

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on GCE"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        --nodes="$node001" \
        --containers="$node001" \
        --image="centos7" \
        --cloud=gce \
        --region="europe-west2-b" \
        --template="n1-highcpu-4" \
        $LOG_OPTION

    retCode=$?
    check_exit_code $?
    if [ $retCode -ne 0 ]; then
        end_verbatim
        return 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [  "$CLUSTER_ID" == 'NOT-FOUND' ]; then
        failure "The cluster '$CLUSTER_NAME' was not found."
        mys9s cluster --list --long
        jobId=$(s9s job --list --batch | tail -n 1 | awk '{print $1}')
        if [ -n "$jobId" ]; then
            mys9s job --log --job-id=$jobId --debug
        fi
    elif [ "$CLUSTER_ID" -gt 0 ]; then
        success "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid."
    fi

    end_verbatim
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local node001="ft-postgresql-gce-01-$$"
    local node001_ip=$(container_ip $node001)
    local node002="ft-postgresql-gce-02-$$"

    print_title "Adding a New Node"
    begin_verbatim

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$node001_ip?master;$node002?slave" \
        --containers="$node002" \
        --image="centos7" \
        --cloud=gce \
        --region="europe-west2-b" \
        --template="n1-highcpu-4" \
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

    containers+="ft-postgresql-gce-01-$$ "
    containers+="ft-postgresql-gce-02-$$"

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
    
        begin_verbatim
        mys9s server \
            --unregister \
            --servers="cmon-cloud://$CMON_CLOUD_CONTAINER_SERVER"

        check_exit_code_no_job $?
        end_verbatim
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
    runFunctionalTest createCluster
    runFunctionalTest testAddNode
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest createServer
    runFunctionalTest createCluster
    runFunctionalTest testAddNode
    runFunctionalTest --force deleteContainer
    runFunctionalTest unregisterServer
fi

endTests
