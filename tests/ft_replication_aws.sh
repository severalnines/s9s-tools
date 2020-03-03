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
OPTION_VENDOR="percona"
PROVIDER_VERSION="5.6"
MY_NODES=""

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
            LOG_OPTION="--log"
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
    local container_name="ft_postgresql_aws_00_$$"
    local class
    local nodeName

    print_title "Creating Container Server"
   
    begin_verbatim

    nodeName=$(create_node --autodestroy $container_name)

    #
    # Creating a cmon-cloud server.
    #
    mys9s server \
        --create \
        --servers="cmon-cloud://$nodeName" \
        $LOG_OPTION

    check_exit_code $?

    CMON_CLOUD_CONTAINER_SERVER="$nodeName"
    mys9s server --list --long
    mys9s container --list --long
    end_verbatim
}

function createCluster()
{
    local node001="ft_replication_aws_node001_$$"
    local node002="ft_replication_aws_haproxy_$$"
    local haproxy_name

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on AWS"
    cat <<EOF | paragraph
  In this test we are creating a mysqlreplication cluster with a HaProxy load
  balancer. The cluster will be created on virtual machines help by the AWS
  cluster. So this test will create a cluster that does the following: (1)
  creates virtual machines on the AWS cluster, (2) creates a replication cluster
  on some of the nodes and (3) creates a HaProxy load balancer on the remaining
  virtual machine.
EOF

    begin_verbatim

    # These are the nodes we are going to destroy at the end of the tests.
    MY_CONTAINERS+=" $node001 $node002"

    mys9s cluster \
        --create \
        --cluster-type=mysqlreplication \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        --nodes="$node001;haProxy://$node002" \
        --containers="$node001;$node002" \
        --vendor="$OPTION_VENDOR" \
        --provider-version="$PROVIDER_VERSION" \
        --cloud=aws \
        --os-user=s9s \
        --generate-key \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    mys9s node --list --long 
    echo "$node001:"
    echo "  Public Ip: $(container_ip $node001)"
    echo "    Private: $(container_ip --private $node001)"
    
    echo "$node002:"
    echo "  Public Ip: $(container_ip $node002)"
    echo "    Private: $(container_ip --private $node002)"

    for i in $(seq 1 10); do
        haproxy_name=$(haproxy_node_name)
        if [ -z "$haproxy_name" ]; then
            warning "Can not find haproxy in cluster."
            mys9s node --list --long
        else
            success "  o HaProxy node is $haproxy_name, ok."
            mys9s node --list --long            
            break
        fi

        mysleep 60
    done

    end_verbatim
}

#
# This will delete the containers we created before.
#
function deleteContainer()
{
    local containers="$MY_CONTAINERS"
    local container

    print_title "Deleting Containers"
    if [ -n "$OPTION_INSTALL" ]; then
        cat <<EOF | paragraph
  The --install option was provided to the test script so containers on the AWS
  cluster will not be deleted. Please delete them manually when they are not
  needed any more.
EOF
    else
        cat <<EOF | paragraph
  Deleting the test containers created on the AWS cloud.
EOF
    fi

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

    mys9s container --list --long
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
