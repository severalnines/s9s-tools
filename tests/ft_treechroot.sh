#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.4"
CONTAINER_SERVER=""

LOG_OPTION="--log"
DEBUG_OPTION="--debug"

CLUSTER_NAME="galera_002"
CLUSTER_ID=""

cd $MYDIR
source include.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]...

  $MYNAME - Creates objects in a chroot CDT environment. 

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:" \
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

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi


#####
# Creating a user to be a normal user. 
#
function testCreateUser()
{
    grant_user
}

#####
# Moving the normal user in the tree.
#
function testMoveUser()
{
    print_title "The user moves itself in CDT"
    begin_verbatim

    TEST_PATH="/home/${PROJECT_OWNER}"
    mys9s tree --mkdir "$TEST_PATH"
    mys9s tree --move /${PROJECT_OWNER} "$TEST_PATH"

    check_exit_code $?

    mys9s tree --list --long
    end_verbatim
}

#
# 
#
function testRegisterServer()
{
    local rootPath="/home/$USER"

    print_title "Registering a Server in Chroot"

    begin_verbatim
    mys9s server \
        --register \
        --cmon-user=${PROJECT_OWNER} \
        --servers="lxc://$CONTAINER_SERVER"

    check_exit_code $?

    mys9s tree --tree

    #
    # Checking.
    #
    OWNER=$(s9s tree --list /$rootPath/$CONTAINER_SERVER --batch --long | head -n1 | awk '{print $3}')
    GROUP=$(s9s tree --list /$rootPath/$CONTAINER_SERVER --batch --long | head -n1 | awk '{print $4}')

    if [ "$OWNER" != "$PROJECT_OWNER" ]; then
        s9s tree --list /$CONTAINER_SERVER 
        failure "The owner is '$OWNER' should be '$USER'."
    else
        success "  o The owner is $OWNER, OK."
    fi

    if [ "$GROUP" != 'users' ]; then
        s9s tree --list /$CONTAINER_SERVER 
        failure "The group is '$GROUP' should be 'users'."
    else
        success "  o The gtoup is '$GROUP', OK."
    fi

    end_verbatim
}

#####
# Creating a container.
#
function testCreateContainer()
{
    local container_name="ft_treechroot_$$"

    print_title "Creating a Container in Chroot"
    
    begin_verbatim
    mys9s container \
        --create \
        --template=ubuntu \
        $LOG_OPTION \
        $container_name

    check_exit_code $?

    CONTAINER_IP=$(\
        s9s server \
            --list-containers \
            --long "$container_name" \
            --batch \
        | awk '{print $6}')

    if [ -z "$CONTAINER_IP" ]; then
        failure "Container IP could not be found."
    else
        success "  o Container IP is $CONTAINER_IP, OK."
    fi

    if [ "$CONTAINER_IP" == "-" ]; then
        failure "Container IP is invalid."
    else
        success "  o Container IP seemt to be valid."
    fi

    node_created "$CONTAINER_IP"

    mys9s tree --list --long
    end_verbatim
}

#####
# Creating a Galera cluster.
#
function testCreateCluster()
{
    local old_ifs="$IFS"
    local n_object_found="0"
    local line
    local name
    local mode
    local owner
    local group

    #
    # Creating a cluster 
    #
    print_title "Creating a Cluster in Chroot"
    begin_verbatim
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$CONTAINER_IP" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version="$PROVIDER_VERSION" \
        $LOG_OPTION

    check_exit_code $?

    CLUSTER_ID=$(find_cluster_id $CLUSTER_NAME)
    if [ "$CLUSTER_ID" -gt 0 ]; then
        success "Cluster ID is $CLUSTER_ID"
    else
        failure "Cluster ID '$CLUSTER_ID' is invalid"
    fi
 
    #
    # Checking the cluster in the tree.
    #
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch /home/$USER); do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case "$name" in
            $CLUSTER_NAME)
                success "  o Cluster $CLUSTER_NAME found, OK."

                if [ "$owner" != "${PROJECT_OWNER}" ]; then
                    failure "Owner is '$owner'."
                else
                    success "  o The owner is $owner, OK."
                fi

                if [ "$group" != "users" ]; then
                    failure "Group is '$group'."
                else
                    success "  o The group is $group, OK."
                fi

                if [ "$mode"  != "crwxrwx---" ]; then
                    failure "Mode is '$mode'." 
                else
                    success "  o The mode is $mode, OK."
                fi

                let n_object_found+=1
                ;;
        esac
    done
    IFS=$old_ifs

    if [ "$n_object_found" -lt 1 ]; then
        failure "Some objects were not found."
    elif [ "$n_object_found" -ne 1 ]; then
        failure "Some objects were not found or duplicate."
    else
        success "Objects found."
    fi

    end_verbatim
}

function dropCluster()
{
    local old_ifs="$IFS"
    local line
    local name

    #
    # Dropping the cluster.
    #
    print_title "Dropping Cluster"
    
    begin_verbatim
    mys9s cluster \
        --drop \
        --cluster-id="$CLUSTER_ID" \
        $LOG_OPTION

    mys9s cluster --list --long 
    mys9s tree --list --long

    #
    # Checking.
    #
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch); do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')

        case "$name" in
            $CLUSTER_NAME)
                failure "Cluster found after it is dropped."
                ;;
        esac
    done
    IFS=$old_ifs  
    
    end_verbatim
}

#
# This will delete the containers we created before.
#
function deleteContainers()
{
    local containers=$(cmon_container_list)
    local container

    print_title "Deleting Containers"
    begin_verbatim

    #
    # Deleting all the containers we created.
    #
    for container in $containers; do
        mys9s container \
            --cmon-user=system \
            --password=secret \
            --delete \
            $LOG_OPTION \
            "$container"
    
        check_exit_code $?
    done

    mys9s tree --list --long
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
        runFunctionalTest testCreateCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateUser
    runFunctionalTest testMoveUser
    runFunctionalTest testRegisterServer
    runFunctionalTest testCreateContainer
    runFunctionalTest testCreateCluster
    runFunctionalTest dropCluster
    runFunctionalTest deleteContainers
fi

endTests


