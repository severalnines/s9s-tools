#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
CONTAINER_SERVER=""

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source include.sh
source shared_test_cases.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check maintenance periods.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o testPing                 Pings the controller.
  o testCreateCluster        Creates a cluster to test on.
  o testCreateMaintenance    Creates a maintenance period.
  o testCreateTwoPeriods     Creates overlapping maintenance periods.
  o testDeletePeriods        Testing the deletion of maintenance.
  o testClusterMaintenance   Testing cluster related maintenance.
  o testDrop                 Dropping the cluster.

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

reset_config

#
# Converts the date to a format the s9s program understands.
#
function dateFormat()
{
    TZ=GMT date -d "$1" "+%Y-%m-%dT%H:%M:%S.000Z"
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Creating a cluster"
    cat <<EOF | paragraph
  Creating a cluster for testing. Any cluster will do, a one node PostgreSQL
  cluster is created fast.
EOF

    begin_verbatim

    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName;"
    FIRST_NODENAME="$nodeName"
    
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
        --provider-version="9.6" \
        $LOG_OPTION
    
    check_exit_code_no_job $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    end_verbatim
}

#
# This test will create a maintenance and it waits until the maintenance is
# expired. Then the test checks if the maintenance period disappeared.
#
function testCreateMaintenance()
{
    local reason="test_$$_maintenance"

    print_title "Creating maintenance expiration"

    begin_verbatim
    
    #
    # Creating a maintenance period that expires real soon.
    #
    mys9s maintenance \
        --create \
        --nodes=$FIRST_NODENAME \
        --start="$(dateFormat "now")" \
        --end="$(dateFormat "now + 10 sec")" \
        --reason="$reason" 

    check_exit_code_no_job $?
    
    #
    # The maintenance period should be there now.
    #
    printVerbose "Maintenance immediately:"
    mys9s maintenance --list --long
    mys9s maintenance --list --print-json

    if ! s9s maintenance --list --long | grep --quiet "$reason"; then
        failure "The maintenance not found with reason '$reason'."
    fi

    #
    # But it should disappear in a jiffy.
    #
    sleep 15
    printVerbose "After 15 seconds '$(date)':"
    mys9s maintenance --list --long

    if s9s maintenance --list --long | grep --quiet "$reason"; then
        failure "The maintenance should have expired: '$reason'."
    fi

    end_verbatim
    return 0
}

function testCreateTwoPeriods()
{
    print_title "Testing overlapping maintenance periods"
    cat <<EOF | paragraph 
  This test will create two overlapping maintenance periods. Then the test will
  wait for a little while and checks that one of the maintenance periods expire,
  the other remains.

EOF

    begin_verbatim

    #
    # Creating a maintenance period that expires real soon and an other one that
    # expires a bit later.
    #
    mys9s maintenance \
        --create \
        --nodes=$FIRST_NODENAME \
        --start="$(dateFormat "now")" \
        --end="$(dateFormat "now + 1 min")" \
        --reason="longer_$$" \
        --batch
    
    check_exit_code_no_job $?

    mys9s maintenance \
        --create \
        --nodes=$FIRST_NODENAME \
        --start="$(dateFormat "now")" \
        --end="$(dateFormat "now + 10 sec")" \
        --reason="shorter_$$" \
        --batch
    
    check_exit_code_no_job $?
    mys9s maintenance --list --long

    #
    # The maintenance periods should be there now.
    #
    if ! s9s maintenance --list --long | grep --quiet "shorter"; then
        failure "The shorter maintenance was not found."
    fi
    
    if ! s9s maintenance --list --long | grep --quiet "longer"; then
        failure "The longer maintenance was not found."
    fi

    #
    # But the short one should disappear in a jiffy.
    #
    mysleep 15
    mys9s maintenance --list --long

    if s9s maintenance --list --long | grep --quiet "shorter"; then
        failure "The shorter maintenance is still found."
    fi
    
    if ! s9s maintenance --list --long | grep --quiet "longer"; then
        failure "The longer maintenance was not found."
    fi

    end_verbatim
    return 0
}

#
# This will go and delete all the maintenance periods.
#
function testDeletePeriods()
{
    #
    # Creating maintenance periods.
    #
    print_title "Testing deleting of maintenance"
    cat <<EOF | paragraph 
  This test will create three maintenance periods then delete them one by one.

EOF
    
    begin_verbatim

    mys9s \
        maintenance --create \
        --nodes=$FIRST_NODENAME \
        --start="$(dateFormat "now")" \
        --end="$(dateFormat "now + 1 day")" \
        --reason="test_1_$$" \
        --batch
    
    check_exit_code_no_job $?

    mys9s \
        maintenance --create \
        --nodes=$FIRST_NODENAME \
        --start="$(dateFormat "now")" \
        --end="$(dateFormat "now + 1 day")" \
        --reason="test_2_$$" \
        --batch
    
    check_exit_code_no_job $?
    
    mys9s \
        maintenance --create \
        --nodes=$FIRST_NODENAME \
        --start="$(dateFormat "now + 1 day")" \
        --end="$(dateFormat "now + 2 days")" \
        --reason="test_3_$$" \
        --batch
    
    check_exit_code_no_job $?
    mys9s maintenance --list --long
    mys9s maintenance --list --long --batch

    #
    # The maintenance periods should be there now, so we can remove them.
    #
    for uuid in $(s9s maintenance --list --batch); do
        mys9s maintenance --delete --uuid=$uuid --batch 
        check_exit_code_no_job $?
    done

    n_maintenances=$(s9s maintenance --list --batch | wc -l)
    if [ "$n_maintenances" -ne 0 ]; then
        failure "There are $n_maintenances remains after the delete."
    else
        success "  o Maintenance periods are deleted, ok."
    fi

    mys9s maintenance --list --long
    end_verbatim

    return 0
}

#
# This test will create a cluster maintenance then wait until it disappears. 
#
function testClusterMaintenance()
{
    local reason="cluster_maintenance_$$"
 
    #
    # Creating a maintenance period that expires real soon.
    #
    print_title "Creating a maintenance period that expires real soon"
    cat <<EOF
  This test will create a cluster maintenance period and wait for a short while
  for it to expire.

EOF

    begin_verbatim

    mys9s \
        maintenance --create \
        --cluster-id=$CLUSTER_ID \
        --start="$(dateFormat "now")" \
        --end="$(dateFormat "now + 5 seconds")" \
        --reason="$reason" \
        --batch
 
    check_exit_code_no_job $?

    #
    # The maintenance periods should be there now.
    #
    mys9s maintenance --list --long

    if ! s9s maintenance --list --long | grep --quiet "$reason"; then
        failure "The maintenance not found: '$reason'."
    else
        success "  o Maintenance found, ok."
    fi
   
    #
    # And should disappear in a few seconds.
    #
    mysleep 10
    mys9s maintenance --list --long

    if s9s maintenance --list --long | grep --quiet "$reason"; then
        failure "The maintenance should have expired: '$reason'."
    else
        success "  o Maintenance expired, ok."
    fi

    end_verbatim
    return 0
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    local exitCode

    print_title "Dropping the cluster"

    begin_verbatim

    #
    # Dropping the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        $LOG_OPTION
    
    check_exit_code_no_job $?

    end_verbatim
    return 0
}

#
# Running the requested tests.
#
startTests
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateMaintenance
    runFunctionalTest testCreateTwoPeriods
    runFunctionalTest testDeletePeriods
    runFunctionalTest testClusterMaintenance
    runFunctionalTest testDrop
fi

endTests


