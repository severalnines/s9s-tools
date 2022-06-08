#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""
PROXY_SERVER=""
CONTAINER_NAME1="${MYBASENAME}_1_$$"

LOG_OPTION="--log"
DEBUG_OPTION="--debug"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

cd $MYDIR
source "./include.sh"

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Checks if a created ProxySql server can be connected.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --provider-version=STRING The SQL server provider version.
 --leave-nodes    Do not destroy the nodes at exit.

EXAMPLE
 ./$MYNAME --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,leave-nodes" \
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

        --leave-nodes)
            shift
            OPTION_LEAVE_NODES="true"
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
    #
    # Creating a galera cluster.
    #
    print_title "Creating a Cluster"

    nodes=$(create_node  --autodestroy $CONTAINER_NAME1)

    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor=percona \
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
}

function testCreateDatabase()
{
    local userName

    print_title "Creating Database"

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="test_database" \
        --batch
    
    check_exit_code_no_job $?
    
    #
    # Creating accounts. They have various allow host settings and access
    # rights.
    # 
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="${PROJECT_OWNER}:password@%" \
        --privileges="*.*:ALL" \
        --batch
    
    check_exit_code_no_job $?

    mys9s node --list --long
}

function testConnect()
{
    local sql_host="$FIRST_ADDED_NODE"
    local sql_port="3306"
    local db_name="test_database"
    local sql_user="${PROJECT_OWNER}"
    local sql_password="password"
    local reply

    print_title "Testing Connection to ${sql_user}@${sql_host}."
    #sql_host="$FIRST_ADDED_NODE"

    #
    # Executing a simple SQL statement using the account we created.
    #
cat <<EOF
        mysql \
            --disable-auto-rehash \
            --batch \
            -h$sql_host \
            -P$sql_port \
            -u$sql_user \
            -p$sql_password \
            $db_name \
            -e "SELECT 41+1"
EOF

    reply=$(\
            mysql \
                --disable-auto-rehash \
                --batch \
                -h$sql_host \
                -P$sql_port \
                -u$sql_user \
                -p$sql_password \
                $db_name \
                -e "SELECT 41+1" \
        | \
            tail -n +2 \
        )
    
    if [ "$reply" != "42" ]; then
        failure "Failed SQL statement on ${sql_user}@${sql_host}: '$reply'."
        exit 1
    fi
}

function testUploadData()
{
    local sql_host="$FIRST_ADDED_NODE"
    local sql_port="3306"
    local db_name="test_database"
    local sql_user="${PROJECT_OWNER}"
    local sql_password="password"

    local reply
    local count=0

    print_title "Restoring mysqldump File"
    cat <<EOF
SQL_USER="$sql_user"
SQL_PASSWORD="$sql_password"
SQL_HOST="$sql_host"
SQL_PORT="$sql_port"
SQL_DATABASE="$db_name"
EOF

    #
    # Here we upload some tables. This part needs test data...
    #
    for file in \
            $HOME/Desktop/stuff/tests/*.sql.gz \
            $HOME/*.sql.gz; 
    do
        if [ ! -f "$file" ]; then
            continue
        fi

        printf "%'6d " "$count"
        printf "$XTERM_COLOR_RED$file$TERM_NORMAL"
        printf "\n"
        pv $file | \
            gunzip | \
            mysql --batch \
                -h$sql_host \
                -P$sql_port \
                -u$sql_user \
                -p$sql_password \
                $db_name

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
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateDatabase
    runFunctionalTest testConnect
    runFunctionalTest testUploadData
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateDatabase
    runFunctionalTest testConnect
    runFunctionalTest testUploadData
fi

endTests


