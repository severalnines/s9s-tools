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
JOB_ID="0"

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
    cat <<EOF
  This test will upload some local file as CDT files so that we can run them in 
the next tests.

EOF

    mys9s tree --mkdir --batch /tests

    for file in scripts/test-scripts/*.js scripts/test-scripts/*.sh; do
        basename=$(basename $file)

        mys9s tree --touch --batch /tests/$basename
        cat $file | s9s tree --save --batch /tests/$basename

        mys9s tree --access --privileges="rwx" "/tests/$basename"
        check_exit_code_no_job $?
    done
}

function testAbortJs()
{
    local file="/tests/imperative_002.js"

    print_title "Aborting a JS Script"
    cat <<EOF
  This test will run a JS script and abort the job while it is running. The
script should be aborted, the job should be in ABORTED state.

EOF

    mys9s tree --cat "$file"
    mys9s script --run "$file"
    let JOB_ID+=1
    sleep 3

    mys9s job --kill --job-id=$JOB_ID
    sleep 2
    mys9s job --list
    
    check_job --job-id $JOB_ID --state ABORTED
}

function testAbortSh()
{
    local file="/tests/shell_script_003.sh"

    print_title "Aborting a Shell Script"
    cat <<EOF
  This test will run a shell script as job and will try to kill the job while
the script is running. Then the job ischecked, it should be aborted.

EOF

    mys9s tree --cat "$file"
    mys9s script --run --timeout=15 "$file"
    let JOB_ID+=1
    sleep 3

    mys9s job --kill --job-id=$JOB_ID
    sleep 3
    mys9s job --list
    
    check_job --job-id $JOB_ID --state ABORTED

    mys9s job --log --job-id=$JOB_ID

    echo "ps aux | grep bash"
    ps aux | grep bash
}


function testRunJsJob()
{
    local exit_code
    local files
    local file

    files="imperative_001.js imperative_002.js imperative_003.js "
    files+="imperative_004.js "

    for file in $files; do
        print_title "Running CDT Script $file"
        cat <<EOF
  This test will run a CDT JS scripts as job. The test will check if the
script is finished successfully, the job is not failing.

EOF
        mys9s tree --cat /tests/$file
        mys9s script --run --log /tests/$file --log-format="%M\n"
        exit_code=$?
        let JOB_ID+=1
        
        check_exit_code $exit_code
    done
}

function testRunShJob()
{
    local exit_code
    local files
    local file

    files="shell_script_001.sh "

    for file in $files; do
        print_title "Running CDT Script $file"
        cat <<EOF
  This test will run a CDT shell scripts as job. The test will check if the
script is finished successfully, the job is not failing.

EOF
        mys9s tree --cat /tests/$file
        mys9s script --run --log /tests/$file --log-format="%M\n"
        exit_code=$?
        let JOB_ID+=1
        
        check_exit_code $exit_code
    done
}

function testRunJsJobFailure()
{
    local exit_code
    local files
    local file

    files="imperative_002.js imperative_006.js imperative_007.js "
    files+="imperative_008.js"

    for file in $files; do
        print_title "Failure in CDT Script $file"
        cat <<EOF
  Here we run a script that should fail. The test checks that The job also 
fail/abort at the end.

EOF

        mys9s tree --cat /tests/$file
        mys9s script --run --log --timeout=5 /tests/$file --log-format="%M\n"
        exit_code=$?
        let JOB_ID+=1
        
        if [ $exit_code -eq 0 ]; then
            failure "The job should fail on the JS script ($exit_code)."
        else
            success "  o Job is failed/aborted, ok"
        fi
    done
}

function testRunShJobFailure()
{
    local exit_code
    local files
    local file

    files="shell_script_002.sh shell_script_003.sh shell_script_004.sh "
    for file in $files; do
        print_title "Failure in CDT Script $file"
        cat <<EOF
  Here we run a script that should fail. The test checks that The job also 
fail/abort at the end.
EOF

        mys9s tree --cat /tests/$file
        mys9s script --run --log --timeout=15 /tests/$file --log-format="%M\n"
        exit_code=$?
        let JOB_ID+=1

        if [ $exit_code -eq 0 ]; then
            failure "The job should fail on the shell script ($exit_code)."
        else
            success "  o Job is failed/aborted, ok"
        fi
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
    nodeName=$(create_node --autodestroy ft_scripts_$$_node0)
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
    local script="./scripts/test-scripts/imperative_001.js"
    local printout
    local name
    local value
    local n_values=0
 
    print_title "Running a Local File as Script"
    cat <<EOF
  This test will run a local file as CJS script on an existing cluster without a
job (immediate, short run). Then the test checks if the scripts output is as it
is expected.

EOF

    mys9s script --execute --cluster-id=1 "$script"

    for printout in $(s9s script --execute --cluster-id=1 $script); do
        name=$(echo "$printout" | awk -F: '{print $3}')
        value=$(echo "$printout" | awk -F: '{print $4}')

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

function testUploadCluster()
{
    print_title "Uploading Scripts for Cluster $CLUSTER_NAME"

    mys9s tree --mkdir --batch /$CLUSTER_NAME/tests
    for file in scripts/test-scripts-cluster/*.js; do
        basename=$(basename $file)

        mys9s tree --touch --batch /$CLUSTER_NAME/tests/$basename
        cat $file | s9s tree --save --batch /$CLUSTER_NAME/tests/$basename

        mys9s tree --access --privileges="rwx" "/$CLUSTER_NAME/tests/$basename"
        check_exit_code_no_job $?
    done
    
}

#
# Here we execute some scripts under the CDT path of the cluster, so the script
# checks the given cluster.
#
function testRunJsJobCluster()
{
    local exit_code
    local files
    local file

    files="imperative_cluster_001.js imperative_cluster_002.js "
    files+="imperative_cluster_003.js imperative_cluster_005.js "

    for file in $files; do
        print_title "Running CDT Script $file"
        cat <<EOF
  This test will run a CDT JS scripts as job under a cluster. The test will 
  check if the script is finished successfully, the job is not failing.

EOF
        mys9s tree --cat /$CLUSTER_NAME/tests/$file
        mys9s script --run --log /$CLUSTER_NAME/tests/$file --log-format="%M\n"
        exit_code=$?
        let JOB_ID+=1
        
        check_exit_code $exit_code
    done
}

function testRegisterServer()
{
    print_title "Registering LXC Server"
    cat <<EOF
  Here we register a container server to check the scripts related to
containers. 

EOF

    mys9s server --register --servers="lxc://$CONTAINER_SERVER"
    check_exit_code_no_job $?
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
        runFunctionalTest testUpload
        runFunctionalTest testAbortJs
        #runFunctionalTest testAbortSh
        runFunctionalTest testRunJsJob
        runFunctionalTest testRunShJob
        runFunctionalTest testRunJsJobFailure
        runFunctionalTest testCreateCluster
        runFunctionalTest testScript01
        runFunctionalTest testUploadCluster
        runFunctionalTest testRunJsJobCluster
        runFunctionalTest testRegisterServer
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testUpload
    runFunctionalTest testAbortJs
    #runFunctionalTest testAbortSh
    runFunctionalTest testRunJsJob
    runFunctionalTest testRunShJob
    runFunctionalTest testRunJsJobFailure
    runFunctionalTest testCreateCluster
    runFunctionalTest testScript01
    runFunctionalTest testUploadCluster
    runFunctionalTest testRunJsJobCluster
    runFunctionalTest testRegisterServer
fi

endTests

