#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
OPTION_INSTALL=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

PROVIDER_VERSION="5.6"
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
  --log            Print the logs while waiting for the job to be ended.
  --server=SERVER  The name of the server that will hold the containers.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --vendor=STRING  Use the given Galera vendor.
  --provider-version=STRING The SQL server provider version.
  --leave-nodes    Do not destroy the nodes at exit.

SUPPORTED TESTS:
  o testPing             Pings the controller.
  o testCreateCluster    Creates a Galera cluster.
  o testSetupAudit       Sets up audit logging.
  o testSetConfig01      Changes some configuration values for the cluster.
  o testSetConfig02      More configuration checks.
  o testRestartNode      Restarts one node of the cluster.
  o testStopStartNode    Stops, then starts a node.
  o testCreateAccount    Creates an account on the cluster.
  o testCreateDatabase   Creates a database on the cluster.
  o testUploadData       If test data is found uploads data to the cluster.
  o testAddNode          Adds a new database node.
  o testAddProxySql      Adds a ProxySql node to the cluster.
  o testAddRemoveHaProxy Adds, then removes a HaProxy node.
  o testAddHaProxy       Adds a HaProxy server to the cluster.
  o testRemoveNode       Removes a data node from the cluster.
  o testRollingRestart   Executes a rolling restart on the cluster.
  o testCreateBackup     Creates a backup.
  o testRestoreBackup    Restores the previously created backup.
  o testRemoveBackup     Removes the backup previously created.
  o testStop             Stops the cluster.
  o testStart            Starts the cluster.

EXAMPLE
 ./$MYNAME --print-commands --server=storage01 --reset-config --install

EOF
    exit 1
}


ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,install,reset-config,\
provider-version:,vendor:,leave-nodes" \
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
    fi
}

function testUpload()
{
    local file

    print_title "Uploading Scripts"

    mys9s tree --mkdir --batch /tests

    for file in scripts/test-scripts/*.js; do
        basename=$(basename $file)

        mys9s tree --touch --batch /tests/$basename
        cat $file | s9s tree --save --batch /tests/$basename

        mys9s tree --access --privileges="rwx" "/tests/$basename"
        check_exit_code_no_job $?
    done
}

function testRunJob()
{
    local exit_code
    local files
    local file

    print_title "Running CDT Scripts"
    cat <<EOF
  This test will run some CDT JS scripts as jobs. The test will check if the
scripts are finished successfully, the jobs are not failing.
EOF

    files="imperative_001.js"
    for file in $files; do
        mys9s tree --cat /tests/$file
        mys9s script --run --log /tests/$file --log-format="%M\n"

        exit_code=$?
        check_exit_code $exit_code
    done
}

#
# This test will allocate a few nodes and install a new cluster.
#
function testCreateCluster()
{
    local nodes
    local nodeName
    local exitCode

    print_title "Creating a Galera Cluster"

    echo "Creating node #0"
    nodeName=$(create_node --autodestroy)
    nodes+="$nodeName;"
    FIRST_ADDED_NODE=$nodeName
    
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

function testScript01()
{
    local script="scripts/test-scripts/imperative_001.js"
    local printout
    local name
    local value
    local n_values=0
 
    print_title "Running a Script"
    cat <<EOF
  This test will run a local file as CJS script on an existing cluster without a
job (immediate, short run). Then the test checks if the scripts output is as it
is expected.

EOF

    mys9s script --execute --cluster-id=1 "$script"

    for printout in $(s9s script --execute --cluster-id=1 $script); do
        name=$(echo "$printout" | awk -F: '{print $1}')
        value=$(echo "$printout" | awk -F: '{print $2}')

        case "$name" in 
            variable1)
                if [ "$value" != "100" ]; then
                    failure "Value for 'variable1' is '$value' not '100'."
                else
                    let n_values+=1
                fi
                ;;

            variable2)
                if [ "$value" != "28" ]; then
                    failure "Value for 'variable2' is '$value' not '28'."
                else
                    let n_values+=1
                fi
                ;;
            
            passed)
                if [ "$value" != "true" ]; then
                    failure "Value for 'passed' is '$value' not 'true'."
                else
                    let n_values+=1
                fi
                ;;

            *)
                failure "Unexpected line '$printout'".
                break
                ;;
        esac
    done

    if [ "$n_values" -ne 3 ]; then
        failure "Expected 3 values, found $n_values."
    fi
}

#
# Running the requested tests.
#
startTests

reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest testUpload
    runFunctionalTest testRunJob
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testUpload
    runFunctionalTest testCreateCluster
    runFunctionalTest testScript01
fi

endTests

