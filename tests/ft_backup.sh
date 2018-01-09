#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
ALL_CREATED_IPS=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""
DATABASE_USER="$USER"
PROVIDER_VERSION="5.7"

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
 --install        Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --provider-version=STRING The SQL server provider version.

EXAMPLE
 ./ft_galera.sh --print-commands --server=storage01 --reset-config --install
EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:" \
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

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode
    local nNodes=1

    print_title "Creating a Galera Cluster"

    for ((n=0;n<nNodes;++n)); do
        echo "Creating container #${n}."
        nodeName=$(create_node)
        nodes+="$nodeName;"
    
        if [ "$n" == "0" ]; then
            FIRST_ADDED_NODE=$nodeName
        fi

        #ALL_CREATED_IPS+=" $nodeName"
    done
       
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
        $LOG_OPTION

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating cluster."
        mys9s job --list
        mys9s job --log --job-id=1
        exit 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    wait_for_cluster_started "$CLUSTER_NAME"

    #mys9s cluster --list --long
    #sleep 60
    #mys9s cluster --list --long
}

#
# Creating a new account on the cluster.
#
function testCreateAccount()
{
    local userName

    print_title "Testing account creation."

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
        exit 1
    fi
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    local userName

    print_title "Testing database creation."

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
        exit 1
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
        --privileges="testCreateDatabase.*:DELETE,TRUNCATE" \
        --batch 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while granting privileges."
        exit 1
    fi

    mys9s account --list --cluster-id=1 --long "$DATABASE_USER"
    return 0
}

#
# The first function that creates a backup.
#
function testCreateBackup01()
{
    local node
    local value

    print_title "Creating a Backup"

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
        --use-pigz \
        --parallellism=5 \
        $LOG_OPTION
    
    check_exit_code $?

    value=$(s9s backup --list --backup-id=1 | wc -l)
    if [ "$value" != 1 ]; then
        failure "There should be 1 backup in the output"
        exit 1
    fi
    
    value=$(s9s backup --list --backup-id=1 --long --batch | awk '{print $3}')
    if [ "$value" != "COMPLETED" ]; then
        failure "The backup should be completed"
        exit 1
    fi

    value=$(s9s backup --list --backup-id=1 --long --batch | awk '{print $4}')
    if [ "$value" != "$USER" ]; then
        failure "The owner of the backup should be '$USER'"
        exit 1
    fi
    
    value=$(s9s backup --list --backup-id=1 --long --batch | awk '{print $2}')
    if [ "$value" != "1" ]; then
        failure "The cluster ID for the backup should be '1'."
        exit 1
    fi

    # Checking the path.
    value=$(\
        s9s backup --list-files --full-path --backup-id=1 | \
        grep '^/tmp/BACKUP-1/mysql/' | \
        wc -l)
    if [ "$value" != 3 ]; then
        failure "Three files should be in '/tmp/BACKUP-1/mysql/'"
        mys9s backup --list-files --full-path --backup-id=1
    fi

    value=$(\
        s9s backup --list-files --full-path --backup-id=1 | \
        grep '^/tmp/BACKUP-1/testCreateDatabase/' | \
        wc -l)
    if [ "$value" != 3 ]; then
        failure "Three files should be in '/tmp/BACKUP-1/testCreateDatabase/'"
        mys9s backup --list-files --full-path --backup-id=1
    fi

    #
    #
    #
    print_title "Verifying Backup 1"
    node=$(create_node)

    mys9s backup \
        --verify \
        --cluster-id=$CLUSTER_ID \
        --backup-id=1 \
        --test-server="$node" \
        $LOG_OPTION
}

function testCreateBackup02()
{
    local node
    print_title "Creating Another Backup"

    #
    # Creating the backup.
    # Using gzip this time.
    #
    mys9s backup \
        --create \
        --title="Second Backup of 'ft_backup.sh'" \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        $LOG_OPTION
    
    check_exit_code $?

    #
    #
    #
    print_title "Verifying Backup 2"
    node=$(create_node)

    mys9s backup \
        --verify \
        --cluster-id=$CLUSTER_ID \
        --backup-id=2 \
        --test-server="$node" \
        $LOG_OPTION
}

function testCreateBackupVerify()
{
    local node

    print_title "Creating and Verifying a Backup"
    node=$(create_node)

    #
    # Creating another backup.
    #
    mys9s backup \
        --create \
        --title="Another backup" \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --test-server="$node" \
        --backup-dir=/tmp \
        $LOG_OPTION
    
    check_exit_code $?

    mys9s backup --list --long
    return 0

    while true; do
        mys9s job --list 
        sleep 10
    done
}

function testCreateBackupMySqlPump()
{
    local node
    print_title "Creating mysqlpump Backup"

    #
    # Creating the backup.
    # Using mysqlpump thid time.
    #
    mys9s backup \
        --create \
        --title="ft_backup.sh mysqlpump backup" \
        --backup-method=mysqlpump \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        $LOG_OPTION
    
    check_exit_code $?
}

function testCreateBackupXtrabackup()
{
    local node
    print_title "Creating xtrabackupfull Backup"

    #
    # Creating the backup.
    # Using thid time.
    #
    mys9s backup \
        --create \
        --title="ft_backup.sh xtrabackupfull backup" \
        --backup-method=xtrabackupfull \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        $LOG_OPTION
    
    check_exit_code $?
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
    runFunctionalTest testCreateAccount
    runFunctionalTest testCreateDatabase
    runFunctionalTest testCreateBackup01
    runFunctionalTest testCreateBackup02
    runFunctionalTest testCreateBackupVerify
    runFunctionalTest testCreateBackupXtrabackup
    # The mysqlpump utility is not even installed.
    #runFunctionalTest testCreateBackupMySqlPump
fi

endTests


