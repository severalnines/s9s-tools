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
PROVIDER_VERSION="10.3"
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
 
  $MYNAME - Test script for s9s to check Galera clusters.

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
  o testStopNode         Stops the first node in the cluster.
  o testCreateBackup     Tries to create a backup.

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

        *) 
            printError "Unhandled option $1."
            break
            ;;
    esac
done

function check_remote_config_file()
{
    local hostname="$FIRST_ADDED_NODE"
    local filename
    local show_file
    local text
    local line

    print_subtitle "Checking Configuration File"

    begin_verbatim
    while [ -n "$1" ]; do
        case "$1" in
            --hostname|--host-name|--host)
                hostname="$2"
                shift 2
                ;;

            --file|--file-name)
                filename="$2"
                shift 2
                ;;

            --show-file)
                show_file="true"
                shift
                ;;

            *)
                break
                ;;
        esac
    done

    if [ -n "$hostname" ]; then
        success "  o Will check host $hostname, ok."
    else
        failure "check_remote_config_file(): Host name expected."
    fi
    
    if [ -n "$filename" ]; then
        success "  o Will check host $filename, ok."
    else
        failure "check_remote_config_file(): File name expected."
    fi

    text=$($SSH $hostname -- cat "$filename")

    for line in "$@"; do
        if echo "$text" | grep --quiet "$line"; then
            success "  o Expresson '$line' found, ok."
        else
            failure "Expression '$line' was not found."
        fi
    done
    
    if [ -n "$show_file" ]; then
        echo "$text" | print_ini_file
    fi
    
    end_verbatim
}

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

    print_title "Creating a Galera Cluster of $OPTION_NUMBER_OF_NODES node(s)"
    cat <<EOF
  This test will create a Galera cluster that we use for testing the controller.
  After creating the cluster the test will wait until the cluster goes into
  STARTED state (with timeout) and check that everything is running and as it
  is expected.

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

    check_remote_config_file \
        --host-name "$FIRST_ADDED_NODE" \
        --file-name "/etc/mysql/my.cnf" \
        "user=mysql" "port=3306"


    #
    # Checking the controller, the nodes and the cluster.
    #
    print_subtitle "Checking the State of the Cluster&Nodes"

    begin_verbatim
    mys9s cluster --stat

    check_controller \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/$CLUSTER_NAME" \
        --status     "CmonHostOnline"
    
    for node in $(echo "$nodes" | tr ';' ' '); do
        check_node \
            --node       "$node" \
            --ip-address "$node" \
            --port       "3306" \
            --config     "/etc/mysql/my.cnf" \
            --owner      "$PROJECT_OWNER" \
            --group      "testgroup" \
            --cdt-path   "/$CLUSTER_NAME" \
            --status     "CmonHostOnline" \
            --no-maint
    done

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "$PROJECT_OWNER" \
        --group      "testgroup" \
        --cdt-path   "/" \
        --type       "GALERA" \
        --state      "STARTED" \
        --config     "/etc/cmon.d/cmon_1.cnf" \
        --log        "/var/log/cmon_1.log"

    end_verbatim

    #
    # One more thing: if the option is given we enable the SSL here, so we test
    # everything with this feature.
    #
    if [ -n "$OPTION_ENABLE_SSL" ]; then
        print_title "Enabling SSL"
        cat <<EOF | paragraph
  This test will enable SSL on a cluster, then the cluster will be stopped and 
  started. Then the test will check if the cluster is indeed started.
EOF
        mys9s cluster --enable-ssl --cluster-id=$CLUSTER_ID \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
        
        mys9s cluster --stop --cluster-id=$CLUSTER_ID \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
        
        mys9s cluster --start --cluster-id=$CLUSTER_ID \
            $LOG_OPTION \
            $DEBUG_OPTION

        check_exit_code $?
    
        wait_for_cluster_started "$CLUSTER_NAME" 
    fi
}

function testCreateAccounts()
{
    local n
    local account_name
    local ln

    print_title "Creating a number of accounts"
    begin_verbatim

    #
    # Creating a number of accounts.
    #
    n=0
    while [ "$n" -lt 25 ]; do
        account_name=$(printf "test_account%03d" "$n")
    
        mys9s account \
            --create \
            --cluster-id=$CLUSTER_ID \
            --account="$account_name:password@1.2.3.4" 

        if [ $? -ne 0 ]; then
            failure "Return code is not 0."
            break
        else
            success "  o Return code is 0, OK."
        fi

        let n+=1
    done


    #
    # Reading back the accounts.
    #
    mys9s account --list --long --cluster-id=$CLUSTER_ID
    ln=$(s9s account --list --long --cluster-id=$CLUSTER_ID --batch | wc -l)
    if [ $ln -gt 20 ]; then
        success "  o Seems to have all the accounts, OK."
    else
        failure "Only received $ln accounts."
    fi

    mys9s account --list --long --cluster-id=$CLUSTER_ID --limit=10
    ln=$(s9s account --list --long --cluster-id=$CLUSTER_ID --limit=10 --batch | wc -l)
    if [ $ln -eq 10 ]; then
        success "  o Got 10 accounts, OK."
    else
        failure "Received $ln accounts instead of 10."
    fi

    mys9s account --list --long --cluster-id=$CLUSTER_ID --limit=10 --offset=10
    ln=$(s9s account --list --long --cluster-id=$CLUSTER_ID --limit=10 --offset=10 --batch | wc -l)
    if [ $ln -eq 10 ]; then
        success "  o Got 10 accounts, OK."
    else
        failure "Received $ln accounts instead of 10."
    fi

    end_verbatim
}


function testStopNode()
{
    local exitCode
   
    print_title "Stopping the Node ($FIRST_ADDED_NODE)"

    begin_verbatim

    #
    # First stop.
    #
    mys9s node \
        --stop \
        --cluster-id=$CLUSTER_ID \
        --nodes=$FIRST_ADDED_NODE \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    end_verbatim
    
    check_remote_config_file \
        --host-name "$FIRST_ADDED_NODE" \
        --file-name "/etc/mysql/my.cnf" \
        "user=mysql" "port=3306"
}

function testCreateBackup()
{
    print_title "Creating xtrabackupfull Backup"
    cat <<EOF | paragraph
  In this test we try to create an xtrabackup backup. It may fail, it doesn't
  matter, what we do is we are checking the mysql configuration file after the
  backup job ended.
EOF

    begin_verbatim

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
        $LOG_OPTION \
        $DEBUG_OPTION

    
    check_remote_config_file \
        --host-name "$FIRST_ADDED_NODE" \
        --file-name "/etc/mysql/my.cnf" \
        "user=mysql" "port=3306"

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
        runFunctionalTest testCreateAccounts
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateAccounts
    runFunctionalTest testStopNode
    #runFunctionalTest testCreateBackup
fi

endTests


