#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--log"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
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
  --install        Create a cluster, some backups and leave them when exiting.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --provider-version=STRING The SQL server provider version.

SUPPORTED TESTS
  o testCreateCluster  Creates a cluster that is needed for the tests.
  o testCreateAccount  Creates an SQL account for the tests.
  o testCreateDatabase Creates a database for the tests.
  o testCreateBackup01 Creates a backup, then verifies it.
  o testCreateBackup02 Creates a backup with gzip this time, verifies it.
  o testCreateBackup03 Creates a backup, then tries to rewrite it.
  o testCreateBackup04 Creates a backup with immediate verification.
  o testCreateBackup05 Creates xtrabackup full, then inc backup.
  o testDeleteBackup   Will delete the backup ID 1.

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

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local container_name
    local nodes
    local nodeName
    local exitCode
    local nNodes=1

    print_title "Creating a Galera Cluster"

    for ((n=0;n<nNodes;++n)); do
        echo "Creating container #${n}."
        container_name="$(printf "ft_backup_%08d_node%02d" "$$" "$n")"
        nodeName=$(create_node --autodestroy $container_name)
        nodes+="$nodeName;"
    
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
    local container_name
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
        --subdirectory="backup-%03i-%04I" \
        --use-pigz \
        --parallellism=5 \
        --encrypt-backup \
        $LOG_OPTION
    
    check_exit_code $?

    value=$(s9s backup --list --backup-id=1 | wc -l)
    if [ "$value" != 1 ]; then
        failure "There should be 1 backup in the output"
        exit 1
    fi
    
    value=$(s9s backup --list --backup-id=1 --long --batch | awk '{print $6}')
    if [ "$value" != "COMPLETED" ]; then
        failure "The backup should be completed"
        exit 1
    fi

    value=$(s9s backup --list --backup-id=1 --long --batch | awk '{print $7}')
    if [ "$value" != "$USER" ]; then
        failure "The owner of the backup should be '$USER'"
        exit 1
    fi
    
    value=$(s9s backup --list --backup-id=1 --long --batch | awk '{print $3}')
    if [ "$value" != "1" ]; then
        failure "The cluster ID for the backup should be '1'."
        exit 1
    fi

    # Checking the path.
    value=$(\
        s9s backup --list-files --full-path --backup-id=1 | \
        grep '^/tmp/backup-001-0001/mysql/' | \
        wc -l)
    if [ "$value" != 1 ]; then
        failure "A file should be in '/tmp/backup-001-0001/mysql/'"
        mys9s backup --list-files --full-path --backup-id=1
        mys9s backup --list-files --full-path 
    fi

    value=$(\
        s9s backup --list-files --full-path --backup-id=1 | \
        grep '^/tmp/backup-001-0001/testCreateDatabase/' | \
        wc -l)
    if [ "$value" != 1 ]; then
        failure "A file should be in '/tmp/backup-001-0001/testCreateDatabase/'"
        mys9s backup --list-files --full-path --backup-id=1
        mys9s backup --list-files --full-path 
    fi

    #
    #
    #
    print_title "Verifying Backup 1"
    container_name="$(printf "ft_backup_%08d_verify%02d" "$$" "2")"
    node=$(create_node --autodestroy "$container_name")

    mys9s backup \
        --verify \
        --cluster-id=$CLUSTER_ID \
        --backup-id=1 \
        --test-server="$node" \
        $LOG_OPTION

    #
    #
    #
    print_title "Printing some info"
    mys9s backup --list
    mys9s backup --list --long

    mys9s backup --list-files
    mys9s backup --list-files --long

    mys9s backup --list-databases
    mys9s backup --list-databases --long
}

function testCreateBackup02()
{
    local container_name
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
    # Verifying the backup.
    #
    print_title "Verifying Backup 2"
    container_name="$(printf "ft_backup_%08d_verify%02d" "$$" "3")"
    node=$(create_node --autodestroy "$container_name")

    mys9s backup \
        --verify \
        --cluster-id=$CLUSTER_ID \
        --backup-id=2 \
        --test-server="$node" \
        $LOG_OPTION
}

#
# Creates a backup, then tries to rewrite it.
#
function testCreateBackup03()
{
    local node
    local retcode

    print_title "Trying to Rewrite Backup"

    #
    # Creating the backup.
    #
    mys9s backup \
        --create \
        --title="testCreateBackup03" \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        --subdirectory="testCreateBackup03" \
        $LOG_OPTION
    
    check_exit_code $?

    mys9s backup \
        --create \
        --title="testCreateBackup03" \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        --subdirectory="testCreateBackup03" \
        --log

    retcode=$?
    if [ "$retcode" -eq 0 ]; then
        failure "Overwriting of backup directory should not be possible"
        exit 1
    else
        echo "Yes, this should have failed, we tried to overwrite the backup."
    fi
}

#
# This test creates a backup and immediately tests it on a test server.
#
function testCreateBackup04()
{
    local container_name
    local node
    local retcode

    print_title "Creating and Verifying a Backup"
    container_name="$(printf "ft_backup_%08d_verify%02d" "$$" "1")"
    node=$(create_node --autodestroy "$container_name")

    mys9s backup --list-files --full-path

    #
    # Creating another backup.
    #
    
    mys9s backup \
        --create \
        --title="Backup with Verification" \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --test-server="$node" \
        --backup-dir=/tmp \
        --subdirectory="backup-%03i-%04I" \
        --log
    
    retcode=$?

    if [ $retcode -ne 0 ]; then
        failure "Creating and verifying backup failed"
        mys9s job --list 
        mys9s backup --list --long 
        exit 1
    fi

    #check_exit_code $retcode

    mys9s backup --list --long
}

function testCreateBackup05()
{
    print_title "Creating xtrabackupfull Backup"

    #
    # Creating the backup.
    # Using xtrabackup this time.
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
    
    print_title "Creating xtrabackupincr Backup"

    #
    # Creating the backup.
    # Using xtrabackup inceremental this time.
    #
    mys9s backup \
        --create \
        --title="ft_backup.sh xtrabackupincr backup" \
        --backup-method=xtrabackupincr \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        $LOG_OPTION
    
    check_exit_code $?
}

function testCreateBackup06()
{
    local container_name
    local node
    print_title "Creating PITR Compatible Backup"

    #
    # Creating the backup.
    # Using gzip this time.
    #
    mys9s backup \
        --create \
        --title="PITR Compatible Backup" \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-dir=/tmp \
        --pitr-compatible \
        $LOG_OPTION
    
    check_exit_code $?

    mys9s backup --list --long
}

function testRestore()
{
    print_title "Restoring Backup 1"

    mys9s backup --list --long 
    mys9s backup \
        --restore \
        --backup-id=1 \
        --cluster-id=1 \
        --log

    check_exit_code $?
    
    #
    #
    #
    print_title "Restoring Backup 7"

    mys9s backup \
        --restore \
        --backup-id=7 \
        --cluster-id=1 \
        --log

    check_exit_code $?
    
    #
    #
    #
    print_title "Restoring Backup 2"

    mys9s backup \
        --restore \
        --backup-id=2 \
        --cluster-id=1 \
        --log

    check_exit_code $?
    
    #
    #
    #
    print_title "Restoring Backup 6"

    mys9s backup \
        --restore \
        --backup-id=6 \
        --cluster-id=1 \
        --log

    check_exit_code $?
}

function testDeleteBackup()
{
    local id
    
    print_title "Deleting Existing Backup"

    #
    # Getting the backup ID of the first backup
    #
    id=$(s9s backup --list --long --backup-format="%I\n" | head -n 1)

    if [ "$id" != "1" ]; then
        failure "Backup with id 1 was not found"
        mys9s backup --list --long
    fi

    #
    # Deleting the backup that we found.
    #
    mys9s backup --delete --backup-id=1

    id=$(s9s backup --list --long --backup-format="%I\n" | head -n 1)
    if [ "$id" == "1" ]; then
        failure "Backup with id 1 still exists"
        mys9s backup --list --long
    fi
   
    return 0
}

function testDeleteOld()
{
   
    # 
    # Here no backup will be deleted because the backup retention is 31 days and
    # we just created these backups.
    #
    print_title "Deleting Old Backups (No Old Backups)"
    mys9s backup --delete-old --cluster-id=1 --job-tags=no_old_found --dry --log
    job_id=$(\
        s9s job --list --batch --job-tags=no_old_found | \
        tail -n 1 | \
        awk '{ print $1 }')

    if ! s9s job --log --job-id=$job_id | grep -q "No old backup records found";
    then
        failure "Job $job_id log does not say what it should say (87235)."
    fi

    #
    # Here no backups will be deleted, because the number of safety copies is
    # much higher than the number of actual backups we have.
    #
    print_title "Deleting Old Backups (Has to Keep All)"
    mys9s backup --delete-old --cluster-id=1 --job-tags=not_enough_sc --backup-retention=0 --safety-copies=10 --dry --log 
    job_id=$(\
        s9s job --list --batch --job-tags=not_enough_sc | \
        tail -n 1 | \
        awk '{ print $1 }')

    if ! s9s job --log --job-id=$job_id | grep -q "no backup can be deleted";
    then
        failure "Job $job_id log does not say what it should say (23387)."
    fi

    #
    # Here some of the existing backups can be deleted, but we are in dry mode
    # anyway.
    #
    print_title "Deleting Old Backups (Has to Keep Some)"
    mys9s backup --delete-old --cluster-id=1 --job-tags=some_deleted --backup-retention=0 --safety-copies=3 --dry --log 
    job_id=$(\
        s9s job --list --batch --job-tags=some_deleted | \
        tail -n 1 | \
        awk '{ print $1 }')

    if ! s9s job --log --job-id=$job_id | grep -q "not considering other expired";
    then
        failure "Job $job_id log does not say what it should say (28737)."
    fi

    #
    #
    #
    print_title "Deleting Old Backups (No Dry)"
    mys9s backup --delete-old --cluster-id=1 --backup-retention=0 --safety-copies=3 --log 
    
    #
    #
    #
    print_title "Deleting Old Backups (Again)"
    mys9s backup --delete-old --cluster-id=1 --backup-retention=0 --safety-copies=3 --log 
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
    runFunctionalTest testCreateBackup01
    runFunctionalTest testCreateBackup02
    runFunctionalTest testCreateBackup03
    runFunctionalTest testCreateBackup04
    runFunctionalTest testCreateBackup05
    runFunctionalTest testCreateBackup06
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
    runFunctionalTest testCreateBackup03
    runFunctionalTest testCreateBackup04
    runFunctionalTest testCreateBackup05
    runFunctionalTest testCreateBackup06
    runFunctionalTest testRestore
    runFunctionalTest testDeleteBackup
    runFunctionalTest testDelete
    runFunctionalTest testDeleteOld
fi

endTests


