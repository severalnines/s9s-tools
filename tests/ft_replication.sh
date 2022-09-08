#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.5"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

OPTION_VENDOR="percona"

# The IP of the node we added last. Empty if we did not.
LAST_ADDED_NODE=""

cd $MYDIR
source ./include.sh
source ./shared_test_cases.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for mysql replication.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=STRING The SQL server provider version.
  --install        Just install the cluster and exit.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,vendor:" \
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
        
        --vendor)
            shift
            OPTION_VENDOR="$1"
            shift
            ;;

        --)
            shift
            break
            ;;
    esac
done

CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}' 2>/dev/null)

if [ -z "$PIP_CONTAINER_CREATE" ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    #
    #
    #
    print_title "Creating MySql Replication Cluster."
    begin_verbatim

    nodeName=$(create_node --autodestroy "${MYBASENAME}_00_$$")
    nodes+="$nodeName?master;"
    FIRST_ADDED_NODE=$nodeName
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_01_$$")
    SECOND_ADDED_NODE=$nodeName
    nodes+="$nodeName?slave;"
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_02_$$")
    THIRD_ADDED_NODE=$nodeName
    nodes+="$nodeName?slave;"
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_03_$$")
    FOURTH_ADDED_NODE=$nodeName
    nodes+="$nodeName?master;"
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_04_$$")
    FIFTH_ADDED_NODE=$nodeName
    nodes+="$nodeName?slave;"
    
    nodeName=$(create_node --autodestroy "${MYBASENAME}_05_$$")
    LAST_ADDED_NODE=$nodeName
    nodes+="$nodeName?slave;"
    
    #
    # Creating a MySQL replication cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=mysqlreplication \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" == "NOT-FOUND" ]; then
        failure "Cluster was not created."
        end_verbatim
        return 1
    elif [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID."
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
        end_verbatim
        return 1
    fi

    wait_for_cluster_started "$CLUSTER_NAME" 

    mys9s replication --list --long

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "${PROJECT_OWNER}" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "REPLICATION" \
        --state      "STARTED" \
        --config     "/etc/cmon.d/cmon_1.cnf" \
        --log        "/var/log/cmon_1.log"

    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$SECOND_ADDED_NODE" \
        --state          "Online"
    
    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$THIRD_ADDED_NODE" \
        --state          "Online"
    
    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$THIRD_ADDED_NODE" \
        --state          "Online"
    
    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$FIFTH_ADDED_NODE" \
        --state          "Online"
    
    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$LAST_ADDED_NODE" \
        --state          "Online"

    end_verbatim
}

function testStageSlave()
{
    print_title "Testing the Rebuilding a Replication Slave"
    cat <<EOF
  This test will use the --stage option to rebuild the replication slave and
  then check if the job was properly executed.

EOF
    begin_verbatim

    mys9s replication --list --long
    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$THIRD_ADDED_NODE" \
        --state          "Online"

    mys9s replication \
        --stage \
        --cluster-id=1 \
        --job-tags="stage" \
        --slave=$THIRD_ADDED_NODE:3306 \
        --master=$FIRST_ADDED_NODE:3306 \
        $LOG_OPTION
    
    check_exit_code $?

    mysleep 60
    mys9s replication --list --long 
    check_replication_state \
        --cluster-name   "$CLUSTER_NAME" \
        --slave          "$THIRD_ADDED_NODE" \
        --state          "Online"

    end_verbatim
}

#
# This test will create a user and a database and then upload some data if the
# data can be found on the local computer.
#
function testUploadData()
{
    local db_name="${PROJECT_OWNER}1"
    local user_name="${PROJECT_OWNER}1"
    local password="p"
    local reply
    local count=0

    print_title "Testing data upload on cluster."
    begin_verbatim

    #
    # Creating a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --db-name=$db_name
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        exit 1
    fi

    #
    # Creating a new account on the cluster.
    #
    mys9s account \
        --create \
        --account="$user_name:$password" \
        --privileges="$db_name.*:ALL"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        exit 1
    fi

    #
    # Executing a simple SQL statement using the account we created.
    #
    reply=$(\
        mysql \
            --disable-auto-rehash \
            --batch \
            -h$FIRST_ADDED_NODE \
            -u$user_name \
            -p$password \
            $db_name \
            -e "SELECT 41+1" | tail -n +2 )

    if [ "$reply" != "42" ]; then
        failure "Cluster failed to execute an SQL statement: '$reply'."
    fi

    #
    # Here we upload some tables. This part needs test data...
    #
    for file in \
        /home/${PROJECT_OWNER}/Desktop/stuff/databases/*.sql.gz \
        /home/domain_names_ngtlds_dropped_whois/*/*/*.sql.gz;
    do
        if [ ! -f "$file" ]; then
            continue
        fi

        printf "%'6d " "$count"
        printf "$XTERM_COLOR_RED$file$TERM_NORMAL"
        printf "\n"
        zcat $file | \
            mysql --batch -h$FIRST_ADDED_NODE -u$user_name -pp $db_name

        exitCode=$?
        if [ "$exitCode" -ne 0 ]; then
            failure "Exit code is $exitCode while uploading data."
            break
        fi

        let count+=1
        if [ "$count" -gt 99 ]; then
            break
        fi
    done

    end_verbatim
}

#
# Stopping the cluster.
#
function testStop()
{
    local exitCode

    print_title "Stopping cluster"
    begin_verbatim

    #
    # Stopping the cluster.
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

    end_verbatim
}

#
# Dropping the cluster from the controller.
#
function testDrop()
{
    print_title "Dropping cluster"
    begin_verbatim

    #
    # Starting the cluster.
    #
    mys9s cluster \
        --drop \
        --cluster-id=$CLUSTER_ID \
        --log 

    if [ $? -eq 0 ]; then
        success "  o The cluster is stopped, ok."
    else
        warning "The drop cluster job has failed."
    fi

    end_verbatim
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user --group "testgroup"

if [ "$OPTION_INSTALL" ]; then
    if [ -n "$1" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    runFunctionalTest testCreateCluster
    runFunctionalTest testUploadData
    runFunctionalTest testStageSlave
    runFunctionalTest testStop
    runFunctionalTest testDrop
fi

endTests


