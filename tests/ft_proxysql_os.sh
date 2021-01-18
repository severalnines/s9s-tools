#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--wait"
DEBUG_OPTION=""

CONTAINER_SERVER=""
CONTAINER_IP=""
CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
PROXYSQL_IP=""
OPTION_INSTALL=""

# Tested 
# percona, 5.6
# percona, 5.7
# mariadb, 10.4
# mariadb, 10.3 FAIL
# mariadb, 10.4 

PROVIDER_VERSION="10.1"
OPTION_VENDOR="mariadb"

NUMBER_OF_CREATED_CONTAINERS="0"
LAST_CONTAINER_IP=""
OPTION_CONTAINER_IMAGE="centos_7"

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh 

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage:
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Testing ProxySql 2.0 installation on various distributions.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --install        Just install the cluster and exit.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:,\
install,proxy2" \
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
            DEBUG_OPTION="--debug"
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

        --install)
            shift
            OPTION_INSTALL="--install"
            ;;

        --proxy2)
            shift
            OPTION_POXYSQL_VERSION="--provider-version=2"
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

function createContainerFromImage()
{
    local container_name
    local container_ip

    let NUMBER_OF_CREATED_CONTAINERS+=1
    container_name=$(printf \
        "${MYBASENAME}_%02d_%06d" \
        $NUMBER_OF_CREATED_CONTAINERS \
        "$$")

    print_subtitle "Creating a Container"
    cat <<EOF | paragraph
  This test actually implements a subtask. It is creating a container on the
  registered container server. Since the test might need multiple containers 
  this might be executed several times. 

EOF

    begin_verbatim

    mys9s container \
        --create \
        --image="$OPTION_CONTAINER_IMAGE" \
        --servers=$CONTAINER_SERVER \
        --log \
        "$container_name"    

    check_exit_code $?
    remember_cmon_container "$container_name"
    
    mys9s container --stat "$container_name"

    check_container "$container_name"
    container_ip=$(container_ip "$container_name")
    if [ -n "$container_ip" ]; then
        success "  o Container with IP $container_ip created, ok."
    else
        failure "Failed to get an IP?"
    fi

    LAST_CONTAINER_IP=$container_ip

    end_verbatim
}

function testCreateCluster()
{
    local ip1
    local ip2
    local ip3
    
    print_title "Creating Galera Cluster"
    cat <<EOF | paragraph
  This test will create a three node Galera cluster on the containers created
  previously.
EOF

    createContainerFromImage
    ip1=$LAST_CONTAINER_IP
    [ -z "$ip1" ] && return 1

    createContainerFromImage
    ip2=$LAST_CONTAINER_IP
    [ -z "$ip2" ] && return 1
    
    createContainerFromImage
    ip3=$LAST_CONTAINER_IP
    [ -z "$ip3" ] && return 1



    #
    # Creating a Galera cluster.
    #
    print_subtitle "Creating the Cluster"
    begin_verbatim
    mys9s cluster \
        --create \
        --job-tags="testCreateCluster" \
        --cluster-type=galera \
        --nodes="$ip1;$ip2;$ip3" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    wait_for_cluster_started "$CLUSTER_NAME"     

    end_verbatim
}

function testAddProxySql()
{
    local ip

    print_title "Adding a ProxySQL Node"
    cat <<EOF | paragraph
  This test installs a ProxySQL node on the previously created container as a
  part of the previously created cluster.

EOF
    
    createContainerFromImage
    ip=$LAST_CONTAINER_IP
    [ -z "$ip" ] && return 1


    print_subtitle "Adding the Node"
    begin_verbatim
    mys9s cluster \
        --add-node --cluster-id=1 \
        --nodes="proxysql://$ip" \
        --provider-version=2 \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    end_verbatim
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    if [ -n "$1" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testRegisterLxcServer
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testRegisterLxcServer
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddProxySql
    runFunctionalTest testDeleteTestContainers
fi

endTests
