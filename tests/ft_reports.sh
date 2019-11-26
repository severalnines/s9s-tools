#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"

LOG_OPTION="--wait"
DEBUG_OPTION=""

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""

PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

OPTION_INSTALL=""
OPTION_NUMBER_OF_NODES="1"
PROVIDER_VERSION="10.4"
OPTION_VENDOR="mariadb"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source ./include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script that creates and manpulates reports.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --leave-nodes    Do not destroy the nodes at exit.
  --enable-ssl     Enable the SSL once the cluster is created.
  
  --provider-version=VERSION The SQL server provider version.
  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:
  o testCreateCluster    Creates a Galera cluster.

EXAMPLE
 ./$MYNAME --print-commands --server=core1 --reset-config --install

EOF
    exit 1
}

OPTIONS="$@"

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

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local node_ip
    local exitCode
    local node_serial=1
    local node_name

    print_title "Creating a Galera Cluster"
    cat <<EOF | paragraph
  This test will create a Galera cluster that we use for testing.
EOF

    begin_verbatim
    
    #
    # Creating a Galera cluster.
    #
    while [ "$node_serial" -le "$OPTION_NUMBER_OF_NODES" ]; do
        node_name=$(printf "${MYBASENAME}_node%03d_$$" "$node_serial")

        echo "Creating node #$node_serial"
        node_ip=$(create_node --autodestroy "$node_name")

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$node_ip"

        if [ -z "$FIRST_ADDED_NODE" ]; then
            FIRST_ADDED_NODE="$node_ip"
        fi

        let node_serial+=1
    done
     
    mys9s cluster \
        --create \
        --job-tags="createCluster" \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME" 
    if [ $? -eq 0 ]; then
        success "  o The cluster got into STARTED state and stayed there, ok. "
    else
        failure "Failed to get into STARTED state."
        mys9s cluster --stat
        mys9s job --list 
    fi
    
    end_verbatim
}

function testReportTemplates()
{
    local report_name
    local expected
    local lines

    print_title "Checking Report Templates"
    cat <<EOF | paragraph
  This test lists the report templates and checks if all expected templates are
  there in the list.
EOF

    begin_verbatim
    mys9s report --list-templates --long
    check_exit_code_no_job $?

    lines=$(s9s report --list-templates --long)

    expected=""
    expected+="default report1 standard testreport availability "
    expected+="backup upgrade schemachange dbgrowth capacity "

    for report_name in $expected; do
        if echo "$lines" | grep --quiet "$report_name"; then
            success "  o Template '$report_name' found, ok."
        else
            failure "Template '$report_name' was not found."
        fi
    done

    end_verbatim
}

function testCreateReport()
{
    print_title "Creating a report"
    cat <<EOF
  This test will create a test report. The test report is designed to check the
  report subsystem in functional tests. The ID of the created report will be 1.
EOF

    begin_verbatim
    mys9s report --create --type=testreport --cluster-id=1
    check_exit_code_no_job $?

    mys9s report --list --long
    check_exit_code_no_job $?

    mys9s report --cat --report-id=1
    check_exit_code_no_job $?

    end_verbatim
}

function testDeleteReport()
{
    local retCode

    print_title "Deleting report"

    begin_verbatim
    mys9s report --delete --report-id=1
    check_exit_code_no_job $?

    mys9s report --list --long
    check_exit_code_no_job $?

    mys9s report --cat --report-id=1
    retCode=$?
    if [ "$retCode" != "0" ]; then
        success "  o Report ID 1 was not found, ok."
    else
        warning "The return code should not be 0."
    fi

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
        runFunctionalTest testCreateCluster
        runFunctionalTest testReportTemplates
        runFunctionalTest testCreateReport
        runFunctionalTest testDeleteReport
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testReportTemplates
    runFunctionalTest testCreateReport
    runFunctionalTest testDeleteReport
fi

endTests


