#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""

LOG_OPTION="--log"
DEBUG_OPTION="--debug"

CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""
DATABASE_USER="$USER"
PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION
OPTION_VENDOR="percona"
OPTION_NUMBER_OF_NODES="2"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""

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
 
  $MYNAME - Test script for s9s to check Galera clusters.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Create a cluster, some backups and leave them when exiting.

  --reset-config      Remove and re-generate the ~/.s9s directory.
  --provider-version=STRING The SQL server provider version.
  --vendor=STRING     Use the given Galera vendor.
  --number-of-nodes=N The number of nodes in the initial cluster.

SUPPORTED TESTS
  o testCreateCluster  Creates a cluster that is needed for the tests.
  o testCreateAccount  Creates an SQL account for the tests.
  o testCreateDatabase Creates a database for the tests.
  o testCreateBackup01 Creates a backup, then verifies it.

EXAMPLE
 ./ft_galera.sh --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
vendor:,provider-version:,number-of-nodes:" \
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

        --vendor)
            shift
            OPTION_VENDOR="$1"
            shift
            ;;

        --number-of-nodes)
            shift
            OPTION_NUMBER_OF_NODES="$1"
            shift
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
    local container_name
    local nodes
    local nodeName
    local exitCode
    local nNodes=$OPTION_NUMBER_OF_NODES

    print_title "Creating a Galera Cluster"

    begin_verbatim
    for ((n=0;n<nNodes;++n)); do
        echo "Creating container #${n}."
        container_name="$(printf "ft_backup_%08d_node0%02d" "$$" "$n")"
        nodeName=$(create_node --autodestroy $container_name)

        if [ -n "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$nodeName"
    
        if [ "$n" == "0" ]; then
            FIRST_ADDED_NODE=$nodeName
        fi
    done
       
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION \
        $DEBUG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"
    mys9s job --log --job-id=2
    mys9s job --log --job-id=3

    #mys9s cluster --list --long
    #sleep 60
    #mys9s cluster --list --long
    end_verbatim
}

function testCreateClusterFromBackup()
{
    local container_name
    local nodes
    local nodeName
    local exitCode
    local nNodes=$OPTION_NUMBER_OF_NODES
    local cluster_name="${CLUSTER_NAME}_copy"


    print_title "Creating a Galera Cluster from Backup"
    cat <<EOF | paragraph
  This test will create a new galera cluster from an existing backup.
EOF

    begin_verbatim
    for ((n=0;n<nNodes;++n)); do
        echo "Creating container #${n}."
        container_name="$(printf "ft_backup_%08d_node1%02d" "$$" "$n")"
        nodeName=$(create_node --autodestroy $container_name)
        nodes+="$nodeName;"
    done

    mys9s debug --list --long
    mys9s cluster --list --long 
    mys9s node --list --long
       
    #
    # Creating a Galera cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$cluster_name" \
        --provider-version=$PROVIDER_VERSION \
        --backup-id=1 \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    
    mys9s cluster --list --long
    mys9s nodes   --list --long
    mys9s job     --list

    wait_for_cluster_started "$cluster_name"
    end_verbatim
}

#
# Creating a new account on the cluster.
#
function testCreateAccount()
{
    local userName

    print_title "Testing account creation."

    begin_verbatim
    #
    # This command will create a new account on the cluster.
    #
    if [ -z "$CLUSTER_ID" ]; then
        failure "No cluster ID found."
        return 1
    fi

    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="$DATABASE_USER:password@1.2.3.4" \
        --with-database
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating an account."
    fi

    mys9s account --list --cluster-id=1 "$DATABASE_USER"
    userName="$(s9s account --list --cluster-id=1 "$DATABASE_USER")"
    if [ "$userName" != "$DATABASE_USER" ]; then
        failure "Failed to create user '$DATABASE_USER'."
    fi
    end_verbatim
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local userName

    print_title "Testing database creation."

    begin_verbatim

    #
    # This command will create a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testCreateDatabase" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
    fi

    mys9s cluster \
        --list-database \
        --long \
        --cluster-id=$CLUSTER_ID 

    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="$DATABASE_USER" \
        --privileges="testCreateDatabase.*:DELETE" \
        --batch 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while granting privileges."
    fi

    mys9s account --list --cluster-id=1 --long "$DATABASE_USER"
    end_verbatim
}

#
# The first function that creates a backup.
#
function testCreateBackup01()
{
    local container_name
    local node
    local value

    print_title "Creating a Backup"

    begin_verbatim
    #
    # Creating the backup.
    #
    mys9s backup \
        --create \
        --title="Backup created by 'ft_backup.sh'" \
        --to-individual-files \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        --subdirectory="backup-%03i-%04I" \
        --use-pigz \
        --parallellism=5 \
        --encrypt-backup \
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
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateAccount
    runFunctionalTest testCreateDatabase
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateAccount
    runFunctionalTest testCreateDatabase
    runFunctionalTest testCreateBackup01
    runFunctionalTest testCreateClusterFromBackup
fi

endTests


