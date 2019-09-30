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
MAXSCALE_IP=""
OPTION_INSTALL=""
OPTION_COLOCATE=""

CONTAINER_NAME1="${MYBASENAME}_11_$$"
CONTAINER_NAME2="${MYBASENAME}_12_$$"
CONTAINER_NAME9="${MYBASENAME}_19_$$"


cd $MYDIR
source include.sh
source shared_test_cases.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
  Usage: $MYNAME [OPTION]... [TESTNAME]

  $MYNAME - Test script for s9s MaxScale support.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --install        Just install the nodes and exit.

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
        --servers="lxc://$CONTAINER_SERVER" \
        --log

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
    else
        success "  o Server class is ok."
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state
}

function createCluster()
{
    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster on LXC"

    mys9s cluster \
        --create \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type=galera \
        --provider-version="5.6" \
        --vendor=percona \
        --cloud=lxc \
        --nodes="$CONTAINER_NAME1" \
        --containers="$CONTAINER_NAME1" \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
}

#
# This test will add a MaxScale node.
#
#
# To connect to MaxScale CLI do on 192.168.0.194:
#maxctrl -u admin -p mariadb
#
#To connect to MaxScale RW: mysql -h192.168.0.194 -uadmin -padmin -P4008
#
#To connect to MaxScale RR: mysql -h192.168.0.194 -uadmin -padmin -P4006
#
function testAddMaxScale()
{
    print_title "Adding a MaxScale Node"
    
    #
    # Adding maxscale to the cluster.
    #
    if [ -z "$OPTION_COLOCATE" ]; then
        # Adding the maxscale host on a new container.
        mys9s cluster \
            --add-node \
            --cluster-id=1 \
            --nodes="maxscale://$CONTAINER_NAME9" \
            --containers="$CONTAINER_NAME9" \
            $LOG_OPTION \
            $DEBUG_OPTION
    
        check_exit_code $?
    else
        # Co-locating the maxscale on a galera node.
        MAXSCALE_IP=$(galera_node_name)
       
        if [ -n "$MAXSCALE_IP" ]; then
            success "  o MaxScale will be installed on $MAXSCALE_IP, ok"

            mys9s cluster \
                --add-node \
                --cluster-id=1 \
                --nodes="maxscale://$MAXSCALE_IP" \
                $LOG_OPTION \
                $DEBUG_OPTION
    
            check_exit_code $?
        else
            failure "Unable to find IP for MaxScale node."
        fi
    fi

    mys9s node --list --long
    MAXSCALE_IP=$(maxscale_node_name)

    #
    #
    #
    print_subtitle "Checking MaxScale State"

    wait_for_node_state "$MAXSCALE_IP" "CmonHostOnline"
}

function testStopContainer()
{
    if [ -n "$OPTION_COLOCATE" ]; then
        return 0
    fi

    #
    #
    #
    print_title "Stopping Container"
    cat <<EOF
  This test will stop the container on which the MaxScale process is running.
  Then the test will check if the controller realizes the MaxScale is down.

EOF

    mys9s container \
        --stop \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$CONTAINER_NAME9"

    check_exit_code $?
    wait_for_node_state "$MAXSCALE_IP" "CmonHostOffLine"
}

function testStartContainer()
{
    if [ -n "$OPTION_COLOCATE" ]; then
        return 0
    fi

    #
    #
    #
    print_title "Starting Container"
    cat <<EOF
  This test will re-start the container which holds the MaxScale process and
  check if the controller figures out the MaxScale is back again.

EOF

    mys9s container \
        --start \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$CONTAINER_NAME9"

    check_exit_code $?
    wait_for_node_state "$MAXSCALE_IP" "CmonHostOnline"
}

function unregisterMaxScale()
{
    local node_ip
    local line
    local retcode

    print_title "Unregistering then Registering MaxScale Node"
    cat <<EOF
  This test will unregister the MaxScale node, then register the same node again
  and check if the node is properly re-added to the cluster.

EOF

    node_ip=$(maxscale_node_name)
   
    #
    # Unregistering by an outsider should not be possible.
    #
    mys9s node \
        --unregister \
        --cmon-user="grumio" \
        --password="p" \
        --nodes="maxscale://$node_ip:6603"
        
    retcode=$?

    if [ "$retcode" -eq 0 ]; then
        failure "Outsiders should not be able to unregister a node."
    else
        success "  o Outsider can't unregister node, ok."
    fi

    #
    # Unregister by the owner should be possible.
    #
    mys9s node \
        --unregister \
        --nodes="maxscale://$node_ip:6603"

    check_exit_code_no_job $?

    mys9s node --list --long
    line=$(s9s node --list --long --batch | grep '^x')
    if [ -z "$line" ]; then 
        success "  o The MaxScale node is no longer part of he cluster, ok."
    else
        failure "The MaxScale is still there after unregistering the node."
    fi

    mysleep 30

    #
    #
    #
    mys9s node \
        --register \
        --cluster-id=1 \
        --nodes="maxscale://$node_ip" \
        --log 

    check_exit_code $?
       
    line=$(s9s node --list --long --batch | grep '^x')
    if [ -n "$line" ]; then 
        success "  o The MaxScale node is part of he cluster, ok."
    else
        failure "The MaxScale is not part of the cluster."
    fi

    wait_for_node_state "$node_ip" "CmonHostOnline"
    mys9s node --list --long
}

function testStopMaxScale()
{
    local node_ip

    print_title "Stopping MaxScale Service"
    cat <<EOF
  This test will stop the MaxScale service and check if the node changed state.

EOF

    node_ip=$(maxscale_node_name)
    if [ -n "$node_ip" ]; then
        success "  o Found the MaxScale node, ok."
    else
        failure "MaxScale node was not found."
        return 1
    fi

    mys9s \
        node \
        --stop \
        --cluster-id=1 \
        --nodes="maxscale://$node_ip:6603" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
    wait_for_node_state "$MAXSCALE_IP" "CmonHostShutDown"
}

function testStartMaxScale()
{
    local node_ip

    print_title "Starting MaxScale Service"
    cat <<EOF
  This test will start the MaxScale service again and check if the node changed
  state.

EOF

    node_ip=$(maxscale_node_name)
    if [ -n "$node_ip" ]; then
        success "  o Found the MaxScale node, ok."
    else
        failure "MaxScale node was not found."
        return 1
    fi

    mys9s \
        node \
        --start \
        --cluster-id=1 \
        --nodes="maxscale://$node_ip:6603" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?    
    wait_for_node_state "$MAXSCALE_IP" "CmonHostOnline"
}


function destroyContainers()
{
    print_title "Destroying Containers"

    mys9s container --delete --wait "$CONTAINER_NAME1"

    if [ -z "$OPTION_COLOCATE" ]; then
        mys9s container --delete --wait "$CONTAINER_NAME9"
    fi
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
        runFunctionalTest testCreateOutsider        
        runFunctionalTest registerServer
        runFunctionalTest createCluster
        runFunctionalTest testAddMaxScale
    fi
elif [ -n "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateOutsider
    runFunctionalTest registerServer
    runFunctionalTest createCluster
    runFunctionalTest testAddMaxScale
    runFunctionalTest testStopContainer
    runFunctionalTest testStartContainer
    runFunctionalTest testStopMaxScale
    runFunctionalTest testStartMaxScale
    runFunctionalTest unregisterMaxScale
    runFunctionalTest destroyContainers
fi

endTests
