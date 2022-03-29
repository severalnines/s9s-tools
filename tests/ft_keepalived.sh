#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.1"

LOG_OPTION="--wait"
DEBUG_OPTION=""
PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

CONTAINER_SERVER=""
CONTAINER_IP=""

MYSQL_NODE1_IP=""
MYSQL_NODE2_IP=""
PROXYSQL_NODE1_IP=""
PROXYSQL_NODE2_IP=""

CMON_CLOUD_CONTAINER_SERVER=""
CLUSTER_NAME="${MYBASENAME}_$$"
PROXYSQL_IP=""
OPTION_INSTALL=""

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME21="${MYBASENAME}_21_$$"
CONTAINER_NAME22="${MYBASENAME}_22_$$"
CONTAINER_NAME9="${MYBASENAME}_19_$$"

cd $MYDIR
source ./include.sh
source ./include_lxc.sh


#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage:
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check the ProxySQL support. 

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --install        Just install the cluster and exit.
  --proxy2         Install ProxySQL 2.0.

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

function createCluster()
{
    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"
    begin_verbatim

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version=$PROVIDER_VERSION \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1;$CONTAINER_NAME2" \
        --containers="$CONTAINER_NAME1;$CONTAINER_NAME2" \
        --template="ubuntu" \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    
    MYSQL_NODE1_IP=$(get_container_ip "$CONTAINER_NAME1")
    MYSQL_NODE2_IP=$(get_container_ip "$CONTAINER_NAME2")
    if [ -n "$MYSQL_NODE1_IP" ]; then
        success "  o MySql node1 IP is $MYSQL_NODE1_IP, Ok."
    else
        failure "Could not find IP for container $CONTAINER_NAME1."
    fi
    
    if [ -n "$MYSQL_NODE2_IP" ]; then
        success "  o MySql node2 IP is $MYSQL_NODE2_IP, Ok."
    else
        failure "Could not find IP for container $CONTAINER_NAME2."
    fi

    end_verbatim
}

function testAddProxySql()
{
    local exitCode

    print_title "Adding ProxySql Nodes"
    begin_verbatim

    #
    # Adding ProxySql to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="proxySql://$CONTAINER_NAME21" \
        --template="ubuntu" \
        --containers="$CONTAINER_NAME21" \
        $LOG_OPTION \
        $DEBUG_OPTION 
    
    exitCode=$?
    check_exit_code $exitCode

    if [ "$exitCode" -ne 0 ]; then
        end_verbatim
        return 1
    fi
   
    mysleep 60
    mys9s cluster \
        --add-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="proxySql://$CONTAINER_NAME22" \
        --template="ubuntu" \
        --containers="$CONTAINER_NAME22" \
        $LOG_OPTION \
        $DEBUG_OPTION 
    
    check_exit_code $?
   
    mys9s node --list --long
    mys9s container --list --long

    PROXYSQL_NODE1_IP=$(get_container_ip "$CONTAINER_NAME21")
    PROXYSQL_NODE2_IP=$(get_container_ip "$CONTAINER_NAME22")
    end_verbatim
}

#
# This test will add a ProxySql node.
#
function testAddKeepalivedFail()
{
    local nodes

    print_title "Trying to a Keepalived Node"
    begin_verbatim

    nodes="keepalived://$PROXYSQL_NODE1_IP"
    nodes+=";keepalived://$PROXYSQL_NODE2_IP"

    #
    # Trying to add ProxySql to the cluster with no eth interface.
    #
    mys9s cluster \
        --add-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="$nodes" \
        --virtual-ip="192.168.42.10" \
        --print-request \
        --log --debug

    if [ $? -eq 0 ]; then
        failure "This job should have failed."
    else
        success "  o Job failed, ok."
    fi
    
    #
    # Trying to add ProxySql to the cluster with no virtual IP address.
    #
    mys9s cluster \
        --add-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="$nodes" \
        --eth-interface="eth0" \
        --print-request \
        --log --debug

    if [ $? -eq 0 ]; then
        failure "This job should have failed."
    else
        success "  o Job failed, ok."
    fi

    end_verbatim

    return 0
}

#
# This test will add a ProxySql node.
#
function testAddKeepalived()
{
    local nodes

    print_title "Adding Keepalived Nodes"
    begin_verbatim

    #
    # Adding ProxySql to the cluster.
    #
    nodes="keepalived://$PROXYSQL_NODE1_IP"
    nodes+=";keepalived://$PROXYSQL_NODE2_IP"

    mys9s cluster \
        --add-node \
        --cluster-name="$CLUSTER_NAME" \
        --nodes="$nodes" \
        --eth-interface="eth0" \
        --virtual-ip="192.168.43.10" \
        --print-request \
        --log --debug

#        $LOG_OPTION \
#        $DEBUG_OPTION 
    
    check_exit_code $?

    mys9s node --list --long
    mys9s node --stat 
    
    mysleep 60
    
    mys9s node --list --long
    mys9s node --stat 


    end_verbatim

    #
    #
    #
#    print_subtitle "Checking Keepalived State"
#    begin_verbatim
#
#    PROXYSQL_IP=$(proxysql_node_name)
#
#    wait_for_node_state "$PROXYSQL_IP" "CmonHostOnline"
#    if [ $? -ne 0 ]; then
#        failure "ProxySql $PROXYSQL_IP is not in CmonHostOnline state"
#    fi
#
#    end_verbatim
}

function unregisterKeepalived()
{
    local line
    local retcode

    print_title "Unregistering Keepalived Node"
    cat <<EOF | paragraph
  This test will unregister the Keepalived node. 
EOF

    begin_verbatim

    #
    # Unregister by the owner should be possible.
    #
    mys9s node \
        --unregister \
        --print-request \
        --nodes="keepalived://$PROXYSQL_NODE1_IP:112"

    check_exit_code_no_job $?
    mys9s node --list --long

#    line=$(s9s node --list --long --batch | grep '^y')
#    if [ -z "$line" ]; then 
#        success "  o The ProxySql node is no longer part of he cluster, ok."
#    else
#        failure "The ProxySql is still there after unregistering the node."
#    fi

    end_verbatim
}

function registerKeepalived()
{
    local line
    local retcode

    print_title "Registering Keepalived Node"
    cat <<EOF | paragraph
  This test will register the Keepalived node that was previously unregistered.
EOF

    begin_verbatim
   
    #
    # Registering the keepalived host here.
    #
    mys9s node \
        --register \
        --cluster-id=1 \
        --print-request \
        --nodes="keepalived://$PROXYSQL_NODE1_IP" \
        --virtual-ip="192.168.42.10" \
        --log 

    check_exit_code $?
    mys9s node --list --long
    
#    mys9s node --list --long
#    line=$(s9s node --list --long --batch | grep '^y')
#    if [ -n "$line" ]; then 
#        success "  o The ProxySql node is part of he cluster, ok."
#    else
#        warning "The ProxySql is not part of the cluster."
#        
#        mysleep 15
#        line=$(s9s node --list --long --batch | grep '^y')
#        if [ -n "$line" ]; then 
#            success "  o The ProxySql node is part of he cluster, ok."
#        else
#            failure "The ProxySql is not part of the cluster."
#        fi
#    fi
#
#    wait_for_node_state "$PROXYSQL_IP" "CmonHostOnline"
    end_verbatim
}


function destroyContainers()
{
    print_title "Destroying Containers"
    begin_verbatim
    mys9s container --delete --wait "$CONTAINER_NAME1"
    mys9s container --delete --wait "$CONTAINER_NAME2"
    mys9s container --delete --wait "$CONTAINER_NAME21"
    mys9s container --delete --wait "$CONTAINER_NAME22"
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
        runFunctionalTest registerServer
        runFunctionalTest createCluster
        runFunctionalTest testAddKeepalived
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testAddProxySql
    runFunctionalTest testAddKeepalivedFail
    runFunctionalTest testAddKeepalived
    runFunctionalTest unregisterKeepalived
    runFunctionalTest registerKeepalived
    runFunctionalTest --force destroyContainers
fi

endTests
