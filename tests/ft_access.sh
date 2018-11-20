#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
OPTION_INSTALL=""

CLUSTER_NAME="galera_001"
CLUSTER_ID=""


PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

OPTION_INSTALL=""
OPTION_NUMBER_OF_NODES="1"
PROVIDER_VERSION="5.6"
OPTION_VENDOR="percona"

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
Usage: 
  $MYNAME [OPTION]...

  $MYNAME - Tests moving objects in the Cmon Directory Tree.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --install           Just install the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,install,reset-config,\
server:" \
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

        --log)
            shift
            LOG_OPTION="--log"
            ;;

        --print-json)
            shift
            OPTION_PRINT_JSON="--print-json"
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

function testCreateUser()
{
    print_title "Creating a User"

    mys9s user \
        --create \
        --cmon-user="system" \
        --password="secret" \
        --group="admins" \
        --create-group \
        --email-address="laszlo@severalnines.com" \
        --first-name="Laszlo" \
        --last-name="Pere"   \
        --generate-key \
        --new-password="pipas" \
        "pipas"

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
        exit 1
    fi

    return 0
}


function testCreateCluster()
{
    local nodes
    local node_ip
    local exitCode
    local node_serial=1
    local node_name

    print_title "Creating a Galera Cluster"
    
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
        exit 1
    fi

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    mys9s cluster --list --long
}

function testOtherUser()
{
    print_title "Creating Another User"
    cat <<EOF
We create a new user. Then we check that this user do not see the already
created cluster because it is not the owner and is in a completely different
user group.

EOF
    mys9s user --create --group=users --new-password=secret fabian
    check_exit_code_no_job $?

    mys9s user --list --long
    if s9s user --list | grep --quiet fabian; then
        success "  o User 'fabian' exists, ok."
    else
        failure "User 'fabian' is not among the users."
    fi

    mys9s cluster --list --long --cmon-user=fabian --password=secret
    if s9s cluster --list --cmon-user=fabian --password=secret | \
        grep --quiet "$CLUSTER_NAME"; 
    then
        failure "The user 'fabian' should not see the cluster."
    else
        success "  o User 'fabian' can't see the cluster, ok."
    fi

    #
    #
    #
    print_title "Adding ACL Entry"
    cat <<EOF
We add an ACL entry to the cluster for the group 'users', then we check that the
user previously created can see the cluster because of this ACL entry.

EOF

    mys9s tree --add-acl --acl="group:users:rw-" "$CLUSTER_NAME"
    check_exit_code_no_job $?

    #mys9s tree --get-acl "$CLUSTER_NAME"

    mys9s cluster --list --long --cmon-user=fabian --password=secret
    if s9s cluster --list --cmon-user=fabian --password=secret | \
        grep --quiet "$CLUSTER_NAME"; 
    then
        success "  o User 'fabian' can see the cluster, ok."
    else
        failure "The user 'fabian' should see the cluster."
    fi
   
    #
    #
    #
    print_title "Removing ACL Entry"
    cat <<EOF
We remove the ACL entry and then check the everything is back, the user do not
see the cluster at all.

EOF

    mys9s tree --remove-acl --acl="group:users:rw-" "$CLUSTER_NAME"
    check_exit_code_no_job $?
    
    mys9s cluster --list --long --cmon-user=fabian --password=secret
    if s9s cluster --list --cmon-user=fabian --password=secret | \
        grep --quiet "$CLUSTER_NAME"; 
    then
        failure "The user 'fabian' should not see the cluster."
    else
        success "  o User 'fabian' can't see the cluster, ok."
    fi
}

#
# Running the requested tests.
#
startTests

reset_config

if [ "$OPTION_INSTALL" ]; then
    if [ "$*" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateUser
        runFunctionalTest testCreateCluster
        runFunctionalTest testOtherUser
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateUser
    runFunctionalTest testCreateCluster
    runFunctionalTest testOtherUser
fi

endTests

