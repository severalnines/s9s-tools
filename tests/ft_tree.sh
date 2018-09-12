#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

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
    local old_ifs="$IFS"
    local columns_found=0

    print_title "Creating a Normal User"
    mys9s user \
        --create \
        --cmon-user="system" \
        --password="secret" \
        --group="users" \
        --create-group \
        --email-address="laszlo@severalnines.com" \
        --first-name="Laszlo" \
        --last-name="Pere"   \
        --generate-key \
        --new-password="pipas" \
        "pipas"

    check_exit_code_no_job $?

    # An extra key for the SSH login to the container.
    mys9s user \
        --add-key \
        --public-key-file="/home/$USER/.ssh/id_rsa.pub" \
        --public-key-name="The SSH key"

    check_exit_code_no_job $?

    #
    #
    #
    mys9s tree --list --long
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch); do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        owner=$(echo "$line" | awk '{print $3}')

        case $name in 
            groups)
                let columns_found+=1
                [ "$owner" != "system" ] && failure "Owner is '$owner'."
                ;;

            admin)
                let columns_found+=1
                [ "$owner" != "admin" ] && failure "Owner is '$owner'."
                ;;

            nobody)
                let columns_found+=1
                [ "$owner" != "nobody" ] && failure "Owner is '$owner'."
                ;;

            pipas)
                let columns_found+=1
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                ;;

            system)
                let columns_found+=1
                [ "$owner" != "system" ] && failure "Owner is '$owner'."
                ;;

            *)
                failure "Unexpected name '$name'."
                ;;
        esac
    done
    IFS=$old_ifs
}

#####
# Creating a user as superuser. Now that we have automatically created admin
# user we don't really need this. 
#
function testCreateSuperuser()
{
    print_title "Creating a user with superuser privileges."
    mys9s user \
        --create \
        --cmon-user="system" \
        --password="secret" \
        --group="admins" \
        --email-address="laszlo@severalnines.com" \
        --first-name="Cmon" \
        --last-name="Administrator"   \
        --generate-key \
        --new-password="admin" \
        "superuser"

    check_exit_code_no_job $?
    mys9s tree --list --long
}

#####
# Registering a server.
#
function testRegisterServer()
{
    local old_ifs="$IFS"
    local object_found
    local line
    local name
    local mode
    local owner
    local group

    print_title "Registering a container server."
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER"

    check_exit_code_no_job $?

    #
    # Checking the tree...
    #
    mys9s tree --list --long
    
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch); do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case $name in 
            $CONTAINER_SERVER)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "srwxrw----" ] && failure "Mode is '$mode'." 
                object_found="yes"
                ;;
        esac
    done
    IFS=$old_ifs    

    if [ -z "$object_found" ]; then
        failure "Object was not found."
    fi
}

#####
# Creating a container.
#
function testCreateContainer()
{
    local old_ifs="$IFS"
    local container_name="ft_tree_01_$$"
    local object_found
    local line
    local name
    local mode
    local owner
    local group
    
    #
    #
    #
    print_title "Creating a Container"
    #pip-container-destroy --server="$CONTAINER_SERVER" ft_tree_001
    #pip-container-destroy --server="$CONTAINER_SERVER" ft_tree_002

    mys9s container \
        --create \
        --template=ubuntu \
        $LOG_OPTION \
        "$container_name"

    check_exit_code $?
    remember_cmon_container "$container_name"

    CONTAINER_IP=$(\
        s9s server \
            --list-containers \
            --long "$container_name" \
            --batch \
        | awk '{print $6}')

    if [ -z "$CONTAINER_IP" ]; then
        failure "Container IP could not be found."
        exit 1
    elif [ "$CONTAINER_IP" == "-" ]; then
        failure "Container IP is invalid."
        exit 1
    fi
    
    mys9s tree --list --long $CONTAINER_SERVER/containers
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch $CONTAINER_SERVER/containers)
    do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case $name in 
            $container_name)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "crwxrw----" ] && failure "Mode is '$mode'." 
                object_found="yes"
                ;;
        esac
    done
    IFS=$old_ifs    

    if [ -z "$object_found" ]; then
        failure "Object was not found."
    fi
}

#####
# Creating a Galera cluster.
#
function testCreateCluster()
{
    local old_ifs="$IFS"
    local cluster_name="galera_001"
    local line
    local name
    local mode
    local owner
    local group

    #
    #
    #
    print_title "Creating a cluster."
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$CONTAINER_IP" \
        --vendor=percona \
        --cluster-name="$cluster_name" \
        --provider-version=5.6 \
        --wait

    check_exit_code $?
    
    #
    #
    #
    mys9s tree --list --long "$cluster_name"
    
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch "$cluster_name"); do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
        [ "$group" != "users" ] && failure "Group is '$group'."
    done
    IFS=$old_ifs    
}

#####
# Creating databases.
#
function testCreateDatabase()
{
    local old_ifs="$IFS"
    local cluster_name="galera_001"
    local n_object_found=0
    local line
    local name
    local mode
    local owner
    local group

    #
    #
    #
    print_title "Creating Databases"
    mys9s cluster \
        --create-database \
        --cluster-name="$cluster_name" \
        --db-name="domain_names_ngtlds_diff" \
        --batch

    check_exit_code_no_job $?

    mys9s cluster \
        --create-database \
        --cluster-name="$cluster_name" \
        --db-name="domain_names_diff" \
        --batch
    
    check_exit_code_no_job $?

    mys9s cluster \
        --create-database \
        --cluster-name="$cluster_name1" \
        --db-name="whois_records_delta" \
        --batch
    
    check_exit_code_no_job $?

    #for n in 1 2 3 4 5 6 7 8 9 10; do
    #    mys9s tree --tree 
    #    sleep 10
    #done
    # FIXME: I am not sure why we need this.
    sleep 15

    mys9s tree --list --long "$cluster_name/databases"
    
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch "$cluster_name/databases")
    do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case "$name" in 
            domain_names_diff)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            domain_names_ngtlds_diff)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            whois_records_delta)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;
        esac
    done
    IFS=$old_ifs    

    if [ "$n_object_found" -lt 3 ]; then
        failure "Some databases were not found."
    fi
}

#####
# Creating a database account.
#
function testCreateAccount()
{
    print_title "Creating Database Account"
    mys9s account \
        --create \
        --cluster-name="galera_001" \
        --account="pipas:pipas" \
        --privileges="*.*:ALL"

    check_exit_code_no_job $?
}

#
# Moving some objects into a sub-folder. The user is moving itself at the last
# step, so this is a chroot environment.
#
function testMoveObjects()
{
    local old_ifs="$IFS"
    local cluster_name="galera_001"
    local n_object_found=0
    local line
    local name
    local mode
    local owner
    local group

    TEST_PATH="/home/pipas"

    #
    #
    #
    print_title "Moving objects into subfolder"

    mys9s tree --mkdir "$TEST_PATH"
    mys9s tree --move /$CONTAINER_SERVER "$TEST_PATH"
    mys9s tree --move /galera_001 "$TEST_PATH"
    mys9s tree --move /pipas "$TEST_PATH"

    mys9s tree --tree 
    
    #
    #
    #
    mys9s tree --list --long --recursive --full-path

    IFS=$'\n'
    for line in $(s9s tree --list --long --recursive --full-path --batch)
    do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        line=$(echo "$line" | sed 's/3, 1/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case "$name" in 
            /$cluster_name)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "crwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /$cluster_name/databases/domain_names_diff)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /$CONTAINER_SERVER)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "srwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /$CONTAINER_SERVER/containers)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "drwxr--r--" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;
        esac
    done

    echo "n_object_found: $n_object_found"
    mys9s tree --list --long --recursive --full-path --cmon-user=system --password=secret

    #
    #
    #
    n_object_found=0
    
    IFS=$'\n'
    for line in $(s9s tree \
        --list --long --recursive --full-path --batch \
        --cmon-user=system --password=secret)
    do
        echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        line=$(echo "$line" | sed 's/3, 1/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')
        case "$name" in 
            /home/pipas/$cluster_name)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "crwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;
        esac
    done
    IFS=$old_ifs    
    
}

#
# Checking the --get-acl in a chroot environment.
#
function testAclChroot()
{
    THE_NAME=$(s9s tree --get-acl --print-json / | jq .object_name)
    THE_PATH=$(s9s tree --get-acl --print-json / | jq .object_path)

    if [ "$THE_PATH" != '"/"' ]; then
        s9s tree --get-acl --print-json / | jq .
        failure "The path should be '/' in getAcl reply not '$THE_PATH'."
        exit 1
    fi

    if [ "$THE_NAME" != '""' ]; then
        s9s tree --get-acl --print-json / | jq .
        failure "The object name should be empty in getAcl reply."
        exit 1
    fi

    THE_NAME=$(s9s tree --get-acl --print-json /galera_001 | jq .object_name)
    THE_PATH=$(s9s tree --get-acl --print-json /galera_001 | jq .object_path)
    if [ "$THE_PATH" != '"/"' ]; then
        s9s tree --get-acl --print-json /galera_001 | jq .
        failure "The path should be '/' in getAcl reply, not '$THE_PATH'."
        exit 1
    fi

    if [ "$THE_NAME" != '"galera_001"' ]; then
        s9s tree --get-acl --print-json /galera_001 | jq .
        failure "The object name should be 'galera_001' in getAcl reply."
        exit 1
    fi

    THE_NAME=$(s9s tree --get-acl --print-json /galera_001/databases | jq .object_name)
    THE_PATH=$(s9s tree --get-acl --print-json /galera_001/databases | jq .object_path)
    if [ "$THE_PATH" != '"/galera_001"' ]; then
        s9s tree --get-acl --print-json /galera_001/databases | jq .
        failure "The path should be '/galera_001' in getAcl reply."
        exit 1
    fi

    if [ "$THE_NAME" != '"databases"' ]; then
        s9s tree --get-acl --print-json /galera_001/databases | jq .
        failure "The object name should be 'databases' in getAcl reply."
        exit 1
    fi
}

#####
# Creating a directory.
#
function testCreateFolder()
{
    print_title "Creating directory"
    mys9s tree \
        --mkdir \
        /tmp

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating account."
        exit 1
    fi
}

#####
# Adding an ACL.
#
function testManipulate()
{
    print_title "Adding an ACL entry"
    mys9s tree --add-acl --acl="user:pipas:rwx" /tmp
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating account."
        exit 1
    fi

    #####
    # Changing the owner
    #
    print_title "Changing the owner"
    mys9s tree --chown --owner=admin:admins /tmp
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating account."
        exit 1
    fi

    mys9s tree --list --directory --long /tmp
    OWNER=$(s9s tree --list --directory --batch --long /tmp | awk '{print $3}')
    GROUP=$(s9s tree --list --directory --batch --long /tmp | awk '{print $4}')
    if [ "$OWNER" != 'admin' ]; then
        failure "The owner should be 'admin' not '$OWNER'."
        exit 1
    fi

    if [ "$GROUP" != 'admins' ]; then
        failure "The group should be 'admins' not '$GROUP'."
        exit 1
    fi
}

#####
# A final view and exit.
#
function testTree()
{
    print_title "Printing tree and ending test"

    mys9s tree --list --color=always
}

#
# This will delete the containers we created before.
#
function deleteContainers()
{
    local containers=$(cmon_container_list)
    local container

    print_title "Deleting Containers"

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

    #mys9s job --list
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
    runFunctionalTest testCreateSuperuser
    runFunctionalTest testRegisterServer
    runFunctionalTest testCreateContainer
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateDatabase
    runFunctionalTest testCreateAccount
    runFunctionalTest testMoveObjects
    runFunctionalTest testAclChroot
    runFunctionalTest testCreateFolder
    runFunctionalTest testManipulate
    runFunctionalTest testTree
    runFunctionalTest deleteContainers
fi

endTests

