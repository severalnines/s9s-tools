#! /bin/bash
MYNAME=$(basename "$0")
MYBASENAME=$(basename "$MYNAME" .sh)
MYDIR=$(dirname "$0")
VERSION="0.0.1"
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
ALL_CREATED_IPS=""
OPTION_INSTALL=""
OPTION_RESET_CONFIG=""

# This is the name of the server that will hold the linux containers.
CONTAINER_SERVER="core1"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - PostgreSQL cluster stop/start test script.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config" \
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

        --)
            shift
            break
            ;;
    esac
done

if [ -z "$S9S" ]; then
    echo "The s9s program is not installed."
    exit 7
fi

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

if [ -z $(which pip-container-create) ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

function reset_config()
{
    local config_dir="$HOME/.s9s"
    local config_file="$config_dir/s9s.conf"

    if [ -z "$OPTION_RESET_CONFIG" ]; then
        return 0
    fi

    printVerbose "Rewriting S9S configuration."
    if [ -d "$config_file" ]; then
        rm -rf "$config_file"
    fi

    if [ ! -d "$config_dir" ]; then
        mkdir "$config_dir"
    fi

    cat >$config_file <<EOF
#
# This configuration file was created by ${MYNAME} version ${VERSION}.
#
[global]
controller = https://localhost:9556

[log]
brief_job_log_format = "%36B:%-5L: %-7S %M\n"
brief_log_format     = "%C %36B:%-5L: %-8S %M\n"
EOF
}

#
# Creates and starts a new virtual machine.
#
function create_node()
{
    local ip

    printVerbose "Creating container..."
    ip=$(pip-container-create --server=$CONTAINER_SERVER)
    printVerbose "Created '$ip'."

    echo $ip
}

function grant_user()
{
    printVerbose "Creating Cmon user ${USER}."
    mys9s user \
        --create \
        --cmon-user=$USER \
        --generate-key \
        >/dev/null 2>/dev/null
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    pip-say "The test to create PostgreSQL cluster is starting now."
    nodeName=$(create_node)
    nodes+="$nodeName:8089;"
    FIRST_ADDED_NODE=$nodeName
    ALL_CREATED_IPS+=" $nodeName"
    
    #
    # Creating a PostgreSQL cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="$nodes" \
        --cluster-name="$CLUSTER_NAME" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version="9.3" \
        $LOG_OPTION

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating cluster."
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
}

#
# This test will add one new node to the cluster.
#
function testAddNode()
{
    local exitCode

    pip-say "The test to add node is starting now."
    printVerbose "Creating node..."

    LAST_ADDED_NODE=$(create_node)
    ALL_CREATED_IPS+=" $LAST_ADDED_NODE"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$FIRST_ADDED_NODE?master;$LAST_ADDED_NODE?slave" \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This test will call a --stop on the cluster. 
#
function testStopCluster()
{
    local exitCode
    local state

    #
    # Stopping the cluster and checking if the cluster state is 'STOPPED'.
    #
    mys9s cluster \
        --stop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    if [ "$state" != "STOPPED" ]; then
        failure "The state should be 'STOPPED' instead of '$state'."
    fi

    #
    # We then show the cluster stat so that the user can see the state.
    #
    #mys9s cluster \
    #    --stat \
    #    --cluster-id=$CLUSTER_ID 
}

#
# This test will call a --start on the cluster. 
#
function testStartCluster()
{
    local exitCode
    local state

    #
    # Starting the cluster and checking if the cluster state is 'STARTED'.
    #
    mys9s cluster \
        --start \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi

    state=$(s9s cluster --list --cluster-id=$CLUSTER_ID --cluster-format="%S")
    if [ "$state" != "STARTED" ]; then
        failure "The state should be 'STARTED' instead of '$state'."
    fi

    #
    # We then show the cluster stat so that the user can see the state.
    #
    #mys9s cluster \
    #    --stat \
    #    --cluster-id=$CLUSTER_ID 
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    pip-say "The test to drop the cluster is starting now."

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This will destroy the containers we created.
#
function testDestroyNodes()
{
    pip-say "The test is now destroying the nodes."
    pip-container-destroy \
        --server=$CONTAINER_SERVER \
        $ALL_CREATED_IPS \
        >/dev/null 2>/dev/null
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testCreateCluster
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testAddNode
    runFunctionalTest testStopCluster
    runFunctionalTest testStartCluster

    runFunctionalTest testDrop
    runFunctionalTest testDestroyNodes
fi

if [ "$FAILED" == "no" ]; then
    pip-say "The test script is now finished. No errors were detected."
else
    pip-say "The test script is now finished. Some failures were detected."
fi

endTests


