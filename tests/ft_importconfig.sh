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
PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION
OPTION_VENDOR="percona"

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
  --print-json     Pring JSON messages.
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --leave-nodes    Do not destroy the nodes at exit.
  
  --provider-version=VERSION The SQL server provider version.
  --number-of-nodes=N        The number of nodes in the initial cluster.

SUPPORTED TESTS:
  o testCreateGalera     Creates a Galera cluster.
  o testCreatePostgresql Creates a PostgreSQL cluster.
  

EXAMPLE
 ./$MYNAME --print-commands --server=core1 --reset-config --install

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,server:,print-commands,install,\
reset-config,\
provider-version:,number-of-nodes:,vendor:,leave-nodes" \
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
            OPTION_VERBOSE="--verbose"
            ;;

        --print-json)
            shift
            OPTION_PRINT_JSON="--print-json"
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

        --)
            shift
            break
            ;;
    esac
done

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateGalera()
{
    local nodes
    local node_ip
    local node_serial=1
    local node_name
    local job_id=1
    local job_state
    local old_ifs="$IFS"

    print_title "Creating Galera Cluster"
    cat <<EOF
EOF

    #
    # Creating some containers.
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
     
    #
    # Creating the cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$nodes" \
        --vendor="$OPTION_VENDOR" \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=$PROVIDER_VERSION \
        $LOG_OPTION

    check_exit_code_no_job $?
    
    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_versions.csv

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_files.csv

    #
    # Checking the cached config files.
    #
    file="/.runtime/files/config_files.csv"

    IFS=$'\n'
    for line in $(s9s tree --cat --cmon-user=system --password=secret $file)
    do
        version=$(echo $line | awk -F, '{print $1}' | tr -d '"')
        committed=$(echo $line | awk -F, '{print $2}' | tr -d '"')
        path=$(echo $line | awk -F, '{print $4}' | tr -d '"')
        if [ "$version" == "Version" ]; then
            continue
        fi

        echo "---------------------------------------------------"
        echo "   version: $version"
        echo " committed: $committed"
        echo "      path: $path"

        if [ "$version" == "0.0.1" ]; then
            success "  o The version is $version, ok"
        else
            failure "The version should not be '$version'."
        fi

        if [ "$committed" == "true" ]; then
            success "  o The file is committed, ok."
        else
            failure "The file should be committed."
        fi

        if [ "$path" == "/etc/mysql/my.cnf" ]; then
            success "  o Path is '$path', ok."
        elif [ "$path" == "/etc/mysql/secrets-backup.cnf" ]; then
            success "  o Path is '$path', ok."
        else
            failure "Path should not be '$path'."
        fi
    done
    IFS=$old_ifs    
}

function testImportConfig()
{
    print_title "Importing Config"

    mys9s cluster --import-config --cluster-id=1 --log
    check_exit_code_no_job $?

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/config_manager

    #mys9s tree --tree \
    #    --cmon-user=system \
    #    --password=secret \
    #    --all /.runtime

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_versions.csv

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_files.csv
}

function testChangeConfig()
{
    print_title "Changing a Configuration File"

    ssh "$FIRST_ADDED_NODE" \
        "echo -e \"\n# testChangeConfig\" | \
         sudo tee --append /etc/mysql/secrets-backup.cnf"

    check_exit_code_no_job $?
    
    mys9s cluster --import-config --cluster-id=1 --log
    check_exit_code_no_job $?
    
    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_versions.csv

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_files.csv
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreatePostgresql()
{
    local node_name="ft_importconfig_11_$$"
    local node_ip
    local lines

    print_title "Creating a PostgreSQL Cluster"
    node_ip=$(create_node --autodestroy "$node_name")

    #
    # Creating a PostgreSQL cluster.
    #
    mys9s cluster \
        --create \
        --cluster-type=postgresql \
        --nodes="$node_ip" \
        --cluster-name="ft_importconfig_psql" \
        --db-admin="postmaster" \
        --db-admin-passwd="passwd12" \
        --provider-version=9.5 \
        $LOG_OPTION

    check_exit_code $?
    
    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
        exit 1
    fi

    mys9s cluster --stat
    mys9s node    --stat

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_versions.csv

    mys9s tree --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/files/config_files.csv 
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
        runFunctionalTest testCreateGalera
        runFunctionalTest testImportConfig
        runFunctionalTest testChangeConfig
        #runFunctionalTest testCreatePostgresql
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateGalera
    runFunctionalTest testImportConfig
    runFunctionalTest testChangeConfig
    runFunctionalTest testCreatePostgresql
fi

endTests


