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
PROVIDER_VERSION="5.6"
N_DATABASE_NODE=1
PROXY_SERVER=""

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
 
  $MYNAME - Test script for s9s to check Galera clusters.

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
 ./ft_galera.sh --print-commands --server=storage01 --reset-config --install

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
#
#
function testPing()
{
    print_title "Pinging controller."

    #
    # Pinging. 
    #
    mys9s cluster --ping 

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
        #pip-say "The controller is off line. Further testing is not possible."
    #else
        #pip-say "The controller is on line."
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
    local n_nodes_added=0

    print_title "Creating a Cluster"

    while true; do
        echo "Creating node #$n_nodes_added"
        nodeName=$(create_node)
        
        if [ "$nodes" ]; then
            nodes+=";"
        fi

        nodes+="$nodeName"

        if [ -z "$FIRST_ADDED_NODE" ]; then
            FIRST_ADDED_NODE="$nodeName"
        fi

        ALL_CREATED_IPS+=" $nodeName"

        #
        #
        #
        let n_nodes_added+=1
        if [ "$n_nodes_added" -ge "$N_DATABASE_NODE" ]; then
            break;
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
        --db-name="test_database" \
        --batch
    
    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        exit 1
    fi
    
    #
    # This command will create a new account on the cluster and grant some
    # rights to the just created database.
    #
    mys9s account \
        --create \
        --cluster-id=$CLUSTER_ID \
        --account="pipas:password" \
        --privileges="*.*:ALL" \
        --batch
    
    check_exit_code_no_job $?
}


#
# This test will add a proxy sql node.
#
function testAddProxySql()
{
    local node
    local nodes
    local nodeName

    print_title "Adding a ProxySQL Node"

    nodeName=$(create_node)
    PROXY_SERVER="$nodeName"
    nodes+="proxySql://$nodeName"
    ALL_CREATED_IPS+=" $nodeName"

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
        $LOG_OPTION
    
    check_exit_code $?
}

function testConnect01()
{
    local sql_host="$PROXY_SERVER"
    local sql_port="6033"
    local db_name="proxydemo"
    local sql_user="proxydemo"
    local sql_password="proxydemo"
    local reply

    print_title "Testing Connection ${sql_user}@${sql_host}."
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

function testConnect02()
{
    local sql_host="$PROXY_SERVER"
    local sql_port="6033"
    local db_name="test_database"
    local sql_user="pipas"
    local sql_password="password"
    local reply

    print_title "Testing Connection ${sql_user}@${sql_host}."
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
    local sql_server
    local db_name="proxydemo"
    local user_name="proxydemo"
    local password="proxydemo"
    
    local reply
    local count=0

    print_title "Restoring mysqldump file."
    #sql_server="$FIRST_ADDED_NODE"
    sql_server="$PROXY_SERVER"

    #
    # Here we upload some tables. This part needs test data...
    #
    for file in /home/pipas/Desktop/stuff/tests/*.sql.gz; do
        if [ ! -f "$file" ]; then
            continue
        fi

        printf "%'6d " "$count"
        printf "$XTERM_COLOR_RED$file$TERM_NORMAL"
        printf "\n"
        pv $file | \
            gunzip | \
            mysql --batch -h$PROXY_SERVER -P6033 -u$user_name -p$password $db_name

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
# This test will add a HaProxy node.
#
function testAddHaProxy()
{
    local node
    local nodes
    
    print_title "Adding a HaProxy Node"

    node=$(create_node)
    nodes+="haProxy://$node"
    ALL_CREATED_IPS+=" $node"

    #
    # Adding haproxy to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$nodes" \
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
    runFunctionalTest testPing

    runFunctionalTest testCreateCluster

    runFunctionalTest testCreateDatabase

    runFunctionalTest testAddProxySql

    runFunctionalTest testConnect01
    #runFunctionalTest testConnect02

    runFunctionalTest testUploadData
fi

endTests


