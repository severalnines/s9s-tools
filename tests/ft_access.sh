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
OPTION_VENDOR="percona"

# The IP of the node we added first and last. Empty if we did not.
FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""
TEST_USER_NAME="laszlo"

cd $MYDIR
source ./include.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]...

  $MYNAME - Test to check who sees the cluster and who won't.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the cluster and exit.
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

function user_should_see_the_cluster()
{
    mys9s cluster --list --long --cmon-user=$TEST_USER_NAME --password=secret
    if s9s cluster --list --cmon-user=$TEST_USER_NAME --password=secret | \
        grep --quiet "$CLUSTER_NAME"; 
    then
        success "  o User '$TEST_USER_NAME' can see the cluster, ok."
    else
        failure "The user '$TEST_USER_NAME' should see the cluster."
    fi
}

function user_should_not_see_the_cluster()
{
    mys9s cluster --list --long --cmon-user=$TEST_USER_NAME --password=secret
    if s9s cluster --list --cmon-user=$TEST_USER_NAME --password=secret | \
        grep --quiet "$CLUSTER_NAME"; 
    then
        failure "The user '$TEST_USER_NAME' should not see the cluster."
    else
        success "  o User '$TEST_USER_NAME' can't see the cluster, ok."
    fi
}

function testCreateUser()
{
    grant_user
}


function testCreateCluster()
{
    local nodes
    local node_ip
    local node_serial=1
    local node_name

    print_title "Creating a Galera Cluster"
    cat <<EOF | paragraph
This test will create a Galera cluster with $OPTION_NUMBER_OF_NODES node(s).

EOF

    begin_verbatim

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
    # Print version, so we know.
    # 
    mys9s --version

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

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        printVerbose "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi

    mys9s cluster --list --long

    check_cluster \
        --cluster         "$CLUSTER_NAME" \
        --owner           "$S9STEST_USER" \
        --group           "users" \
        --cdt-path        "/"

    end_verbatim
}

function testOtherUser()
{
    print_title "Creating Another User"
    cat <<EOF
We create a new user. Then we check that this user do not see the already
created cluster because it is not the owner and is in a completely different
user group.

EOF

    begin_verbatim
    mys9s user \
        --create \
        --group=users \
        --new-password=secret \
        --email-address=${TEST_EMAIL} \
        --first-name="Laszlo" \
        --last-name="Pere"   \
        "$TEST_USER_NAME"

    check_exit_code_no_job $?

    mys9s user --list --long
    if s9s user --list | grep --quiet $TEST_USER_NAME; then
        success "  o User '$TEST_USER_NAME' exists, ok."
    else
        failure "User '$TEST_USER_NAME' is not among the users."
    fi

    user_should_not_see_the_cluster
    end_verbatim

    #
    #
    #
    print_title "Adding Group ACL Entry"
    cat <<EOF
We add an ACL entry to the cluster for the group 'users', then we check that the
user previously created can see the cluster because of this ACL entry.

EOF

    begin_verbatim
    
    mys9s tree --add-acl --acl="group:users:rw-" "$CLUSTER_NAME"
    check_exit_code_no_job $?
    user_should_see_the_cluster

    end_verbatim

    #
    #
    #
    print_title "Removing Group ACL Entry"
    cat <<EOF
We remove the ACL entry and then check the everything is back, the user do not
see the cluster at all.

EOF

    begin_verbatim
    mys9s tree --remove-acl --acl="group:users:rw-" "$CLUSTER_NAME"
    check_exit_code_no_job $?
    user_should_not_see_the_cluster
    end_verbatim

    #
    #
    #
    print_title "Adding User ACL Entry"
    cat <<EOF
We add an ACL entry to the cluster for the user '$TEST_USER_NAME', then we check that the
user previously created can see the cluster because of this ACL entry.

EOF

    begin_verbatim
    mys9s tree --add-acl --acl="user:$TEST_USER_NAME:rw-" "$CLUSTER_NAME"
    check_exit_code_no_job $?
    user_should_see_the_cluster
    end_verbatim

    #
    #
    #
    print_title "Removing User ACL Entry"
    cat <<EOF
We remove the ACL entry and then check the everything is back, the user do not
see the cluster at all.

EOF
    
    begin_verbatim
    mys9s tree --remove-acl --acl="user:$TEST_USER_NAME:rw-" "$CLUSTER_NAME"
    check_exit_code_no_job $?
    user_should_not_see_the_cluster
    end_verbatim
}

function testAddGroup()
{
    local tmp

    print_title "Adding User to Second Group"
    cat <<EOF
This test will add the user to the 'admins' group as secondary group while the
primary group remains the 'users' group. Then the access to the cluster is 
checked.

EOF

    begin_verbatim
    mys9s user \
        --add-to-group \
        --group=admins \
        "$TEST_USER_NAME"

    check_exit_code_no_job $?

    mys9s user --stat "$TEST_USER_NAME"
    tmp=$(s9s user --list ${TEST_USER_NAME} --user-format="%G")
    if [ "$tmp" == "users,admins" ]; then
        success "  o User is a member of the admins group, ok."
    else
        failure "The user's groups should not be '$tmp'."
    fi

    user_should_see_the_cluster
    end_verbatim

    #
    #
    #
    print_title "Removing User from Group"
    cat <<EOF
Removing user from the 'admins' group, checking the access rights.

EOF

    begin_verbatim
    mys9s user \
        --remove-from-group \
        --group=admins \
        "$TEST_USER_NAME"

    check_exit_code_no_job $?
    user_should_not_see_the_cluster
    end_verbatim
}

function testChOwn()
{
    print_title "Changing the Ownership"

    begin_verbatim
    s9s tree \
        --chown \
        --recursive \
        --owner=$TEST_USER_NAME:users \
        "/$CLUSTER_NAME"

    check_exit_code_no_job $?
    user_should_see_the_cluster

    s9s tree \
        --chown \
        --recursive \
        --owner=${S9STEST_ADMIN_USER}:admins \
        "/$CLUSTER_NAME"

    check_exit_code_no_job $?
    user_should_not_see_the_cluster
    end_verbatim
}

function testDeleteUser()
{
    print_title "Deleting User"
   
    begin_verbatim
    mys9s tree \
        --chown \
        --recursive \
        --owner=$TEST_USER_NAME:users \
        "/$CLUSTER_NAME"

    mys9s tree --list --long --recursive --all

    #
    # Deleting the user, printing the tree.
    #
    mys9s user --delete "$TEST_USER_NAME"
    check_exit_code_no_job $?

    mys9s tree --list --long --recursive --all 

    #
    # Checking if the tree changed, the owner is system for the objects owned by
    # the deleted user.
    #
    check_controller \
        --owner      "system" \
        --group      "users" 

    check_node \
        --node       "$FIRST_ADDED_NODE" \
        --owner      "system" \
        --group      "users" 

    check_cluster \
        --cluster    "$CLUSTER_NAME" \
        --owner      "system" \
        --group      "users" 

    end_verbatim
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
        runFunctionalTest testAddGroup
        runFunctionalTest testChOwn
        runFunctionalTest testDeleteUser
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateUser
    runFunctionalTest testCreateCluster
    runFunctionalTest testOtherUser
    runFunctionalTest testAddGroup
    runFunctionalTest testChOwn
    runFunctionalTest testDeleteUser
fi

endTests

