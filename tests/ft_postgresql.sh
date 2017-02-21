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
Usage: $MYNAME [OPTION]... [TESTNAME]
 Test script for s9s to check various error conditions.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:" \
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

CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

if [ -z $(which pip-container-create) ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

#
# Creates and starts a new 
#
function create_node()
{
    local ip

    ip=$(pip-container-create --server=$CONTAINER_SERVER)
    echo $ip
}

#
# $1: the name of the cluster
#
function find_cluster_id()
{
    local name="$1"
    local retval
    local nTry=0

    while true; do
        retval=$($S9S cluster --list --long --batch --cluster-name="$name")
        retval=$(echo "$retval" | awk '{print $1}')

        if [ -z "$retval" ]; then
            printVerbose "Cluster '$name' was not found."
            let nTry+=1

            if [ "$nTry" -gt 10 ]; then
                echo 0
                break
            else
                sleep 3
            fi
        else
            printVerbose "Cluster '$name' was found with ID ${retval}."
            echo "$retval"
            break
        fi
    done
}

function grant_user()
{
    $S9S user --create --cmon-user=$USER --generate-key \
        >/dev/null 2>/dev/null
}

#
#
#
function testPing()
{
    pip-say "Pinging controller."

    #
    # Pinging. 
    #
    $S9S cluster --ping 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
        pip-say "The controller is off line. Further testing is not possible."
    else
        pip-say "The controller is on line."
    fi
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
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    ALL_CREATED_IPS+=" $nodeName"
    
    #
    # Creating a PostgreSQL cluster.
    #
    $S9S cluster \
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
# 
#
function testConfig()
{
    local exitCode
    
    pip-say "The test to check configuration is starting now."

    #
    # Listing the configuration values.
    #
    cat <<EOF
    $S9S node 
        --list-config 
        --nodes=$FIRST_ADDED_NODE:5432 
EOF

    $S9S node \
        --list-config \
        --nodes=$FIRST_ADDED_NODE:5432 \

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# Creating a new account on the cluster.
#
#
function testCreateAccount()
{
    pip-say "Testing account creation."

    #
    # This command will create a new account on the cluster.
    #
    $S9S cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="joe:password" \
        --with-database \
        --batch 
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating an account."
    fi
    
    #
    # This command will delete the same account from the cluster.
    #
    $S9S cluster \
        --delete-account \
        --cluster-id=$CLUSTER_ID \
        --account="joe" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while deleting an account."
    fi
}

#
# Creating a new database on the cluster.
#
function testCreateDatabase()
{
    pip-say "Testing database creation."

    #
    # This command will create a new account on the cluster.
    #
    $S9S cluster \
        --create-database \
        --cluster-id=$CLUSTER_ID \
        --db-name="testCreateDatabase" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating a database."
    fi
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    $S9S cluster \
        --create-account \
        --cluster-id=$CLUSTER_ID \
        --account="pipas:password" \
        --privileges="testCreateDatabase.*:INSERT,UPDATE" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while creating account."
    fi
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    $S9S cluster \
        --grant \
        --cluster-id=$CLUSTER_ID \
        --account="pipas" \
        --privileges="testCreateDatabase.*:DELETE,TRUNCATE" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while granting privileges."
    fi
}

#
# This will create a backup.
#
function testCreateBackup()
{
    local exitCode
    
    pip-say "The test to create a backup is starting."

    #
    # Creating a backup.
    #
    $S9S backup \
        --create \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        --backup-directory=/tmp \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This will restore a backup. 
#
function testRestoreBackup()
{
    local exitCode
    local backupId

    pip-say "The test to restore a backup is starting."
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        awk '{print $1}')

    #
    # Calling for a rolling restart.
    #
    $S9S backup \
        --restore \
        --cluster-id=$CLUSTER_ID \
        --backup-id=$backupId \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
}

#
# This will remove a backup. 
#
function testRemoveBackup()
{
    local exitCode
    local backupId

    pip-say "The test to remove a backup is starting."
    backupId=$(\
        $S9S backup --list --long --batch --cluster-id=$CLUSTER_ID |\
        awk '{print $1}')

    #
    # Calling for a rolling restart.
    #
    $S9S backup \
        --delete \
        --backup-id=$backupId \
        --batch \
        $LOG_OPTION
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"
    fi
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
    $S9S cluster \
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
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testPing
    
    runFunctionalTest testCreateCluster
    runFunctionalTest testConfig

    runFunctionalTest testCreateAccount
    runFunctionalTest testCreateDatabase

    runFunctionalTest testCreateBackup
    runFunctionalTest testRestoreBackup
    runFunctionalTest testRemoveBackup

    runFunctionalTest testDrop
    runFunctionalTest testDestroyNodes
fi

endTests


