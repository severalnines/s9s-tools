#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

PROVIDER_VERSION="5.7"
OPTION_VENDOR="percona"

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

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
 
  $MYNAME - Tests stopping and starting a mysqlreplication cluster.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --leave-nodes    Do not destroy the nodes at exit.
  --enable-ssl     Enable the SSL once the cluster is created.
  
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=VERSION The SQL server provider version.

  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:

EXAMPLE
 ./$MYNAME --print-commands --server=core1 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,number-of-nodes:,vendor:,leave-nodes,enable-ssl" \
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
            DEBUG_OPTION="--debug"
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

        --number-of-nodes)
            shift
            OPTION_NUMBER_OF_NODES="$1"
            shift
            ;;
        
        --vendor)
            shift
            OPTION_VENDOR="$1"
            shift
            ;;

        --leave-nodes)
            shift
            OPTION_LEAVE_NODES="true"
            ;;

        --enable-ssl)
            shift
            OPTION_ENABLE_SSL="true"
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
    local node001="${MYBASENAME}_01_$$"
    local node002="${MYBASENAME}_02_$$"
    local node003="${MYBASENAME}_03_$$"

    #
    # Creating a Cluster.
    #
    print_title "Creating a Cluster"

    begin_verbatim
    date

    mys9s cluster \
        --create \
        --template="ubuntu" \
        --cluster-name="$CLUSTER_NAME" \
        --cluster-type="mysqlreplication" \
        --provider-version="$PROVIDER_VERSION" \
        --vendor=$OPTION_VENDOR \
        --nodes="$node001;$node002;$node003" \
        --containers="$node001;$node002;$node003" \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    CLUSTER_ID="1"
    
    date

    check_container_ids --galera-nodes

    wait_for_cluster_started "$CLUSTER_NAME"

    mys9s node --list --long
    mys9s node --stat
    mys9s cluster --list --long
    mys9s cluster --stat
   
    end_verbatim
}

function stopCluster()
{
    print_title "Stopping the Cluster"

    begin_verbatim
    mys9s job --list
    mys9s cluster --stop --log --cluster-id=1
    check_exit_code $?

    #mys9s maintenance --list --long
    mys9s job --list
    mys9s cluster --stat
    mys9s node --list --long
    #mys9s node --stat
    end_verbatim

    #
    #
    #
    print_title "Waiting for a while"
    cat <<EOF | paragraph
  The test will now wait for a while to see if the cluster remains in stopped
  state.
EOF

    begin_verbatim
    mysleep 120
    mys9s job --list
    mys9s cluster --stat
    mys9s node --list --long
    
    end_verbatim
}

function startCluster()
{
    print_title "Starting the Cluster"

    begin_verbatim
    mys9s node --stat

    mys9s cluster --start --log --cluster-id=1
    check_exit_code $?

    #mys9s maintenance --list --long
    mys9s cluster --stat
    mys9s node --list --long
    wait_for_cluster_started "$CLUSTER_NAME"
    end_verbatim
}

function stopContainers()
{
    
    local node001="${MYBASENAME}_01_$$"
    local node002="${MYBASENAME}_02_$$"
    local node003="${MYBASENAME}_03_$$"

    print_title "Stopping Containers"

    begin_verbatim
    for container in $node001 $node002 $node003; do
        mys9s container --stop $container --wait
    done

    mys9s container --list --long
    end_verbatim
}

function startContainers()
{
    
    local node001="${MYBASENAME}_01_$$"
    local node002="${MYBASENAME}_02_$$"
    local node003="${MYBASENAME}_03_$$"

    print_title "Stopping Containers"

    begin_verbatim
    for container in $node001 $node002 $node003; do
        mys9s container --start $container --wait
    done

    mys9s container --list --long
    #mysleep 120
    end_verbatim
}

function deleteContainers()
{
    
    local node001="${MYBASENAME}_01_$$"
    local node002="${MYBASENAME}_02_$$"
    local node003="${MYBASENAME}_03_$$"

    print_title "Stopping Containers"

    begin_verbatim
    for container in $node001 $node002 $node003; do
        mys9s container --delete $container --wait
    done

    mys9s container --list --long
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
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testRegisterServer
    runFunctionalTest createCluster

    runFunctionalTest stopCluster
    runFunctionalTest stopContainers
    runFunctionalTest startContainers
    runFunctionalTest startCluster

    runFunctionalTest deleteContainers
fi

endTests
