#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
MYHOSTNAME="$(hostname)"
VERBOSE=""
VERSION="0.0.3"

LOG_OPTION="--log"
DEBUG_OPTION="--debug"

CONTAINER_SERVER="$MYHOSTNAME"
CONTAINER_IP=""

CLUSTER_NAME="${MYBASENAME}_$$"
LAST_CONTAINER_NAME=""

cd $MYDIR
source include.sh
source shared_test_cases.sh

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
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --install        Just install the server and the cluster and exit.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.

SUPPORTED TESTS:
  o registerServer     Registers a new container server. No software installed.
  o testCreateOutsider Creates an outsider user for checks.
  o checkServer        Checking the previously registered server.
  o createContainer    Creates a new container.
  o createAsSystem     Create a container as system user.
  o createFail         A container creation that should fail.
  o createContainers   Creates several new containers with various images.
  o restartContainer   Stop then start the container.
  o createServer       Creates a server from the previously created container.
  o failOnContainers   Testing on unexisting containers.
  o deleteContainer    Deletes the previously created container.

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
            DEBUG_OPTION="--debug"
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

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

if [ -z "$CONTAINER_SERVER" ]; then
    printError "No container server specified."
    printError "Use the --server command line option to set the server."
    exit 6
fi

#
# This will register the container server. 
#
function registerServer()
{
    local class

    print_title "Registering Container Server"
    cat <<EOF
  This test will simply register a container server and check that the operation
  was indeed successful (the s9s returns with 0). Then the state and the type of
  the registered server is checked.
EOF

    begin_verbatim

    #
    # Creating a container.
    #
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonLxcServer \
        --server-name  "$CONTAINER_SERVER" \
        --cloud        "lxc"

    end_verbatim
}

#
# Checking the previously registered container server.
#
function checkServer()
{
    local old_ifs="$IFS"
    local myhostname="$CONTAINER_SERVER"
    local line
    local cloud
    local region
    local server
    local cidr
    local vpc 
    local id

    #
    #
    #
    print_title "Checking Subnets"

    begin_verbatim

    mys9s server --list-subnets --long

    IFS=$'\n'
    for line in $(s9s server --list-subnets --long --batch); do
        cloud=$(echo "$line" | awk '{print $1}')
        region=$(echo "$line" | awk '{print $2}')
        hostname=$(echo "$line" | awk '{print $3}')
        cidr=$(echo "$line" | awk '{print $4}')
        vpc=$(echo "$line" | awk '{print $5}')
        id=$(echo "$line" | awk '{print $6}')

        if [ "$cloud" != "lxc" ]; then
            failure "The cloud is '$cloud' instead of 'lxc'"
        fi
        
        if [ "$region" != "region1" ]; then
            failure "The region is '$region' instead of 'region1'"
        fi

        if [ "$hostname" != "$myhostname" ]; then
            failure "The hostname is '$hostname' instead of '$myhostname'."
        fi

    done
    IFS="$old_ifs"
    
    end_verbatim

    #
    #
    #
    print_title "Check templates"
    begin_verbatim

    mys9s server --list-templates --long
    
    IFS=$'\n'
    for line in $(s9s server --list-templates --long --batch); do
        cloud=$(echo "$line" | awk '{print $1}')
        region=$(echo "$line" | awk '{print $2}')
        hostname=$(echo "$line" | awk '{print $5}')

        if [ "$cloud" != "lxc" ]; then
            failure "The cloud is '$cloud' instead of 'lxc'"
        fi

        if [ "$region" != "region1" ]; then
            failure "The region is '$region' instead of 'region1'"
        fi
        
        if [ "$hostname" != "$myhostname" ]; then
            failure "The hostname is '$hostname' instead of '$myhostname'."
        fi
    done
    IFS="$old_ifs"

    end_verbatim
}

function checkServerAccess()
{
    local container_name="ft_containers_lxc_xx_$$"    
    local line
    local retcode

    print_title "Checking the Access to the Server"
    cat <<EOF
  Checking the access to the newly registered container server. Checked with the
  owner and an outsider.
EOF

    begin_verbatim

    # With the owner...
    mys9s server --list --long
    line=$(s9s server --list --long | tail -n1)

    check_exit_code_no_job $?
    if echo "$line" | grep --quiet "1 server"; then
        success "  o Owner can see the server, ok."
    else
        failure "Owner can't see the server."
    fi
    
    # With an outsider...
    mys9s server --list --long --cmon-user="grumio" --password="p"
    line=$(s9s server --list --long --cmon-user="grumio" --password="p" \
        | tail -n1)
    
    check_exit_code_no_job $?    
    if echo "$line" | grep --quiet "0 server"; then
        success "  o Outsiders can't see the server, ok."
    else
        failure "Outsider can see the server."
    fi
    
    end_verbatim

    #
    #
    #
    print_title "Checking the Access to the Containers"
    cat <<EOF
  Checking the access to the container list on the newly registered container
  server. Tested with the user and with an outsider too.
EOF

    begin_verbatim

    mys9s container --list --long
    line=$(s9s container --list --long | tail -n1)

    check_exit_code_no_job $?
    if echo "$line" | grep --quiet " 0 container"; then
        failure "Owner can't see the containers."
    else
        success "  o Owner can see the containers, ok."
    fi
    
    # With an outsider...
    mys9s container --list --long --cmon-user="grumio" --password="p"
    line=$(s9s container --list --long --cmon-user="grumio" --password="p" \
        | tail -n1)
    
    check_exit_code_no_job $?    
    if echo "$line" | grep --quiet " 0 container"; then
        success "  o Outsiders can't see the containers, ok."
    else
        warning "Outsider can see the containers."
    fi
    
    s9s tree --list --long --recursive --full-path

    end_verbatim

    #
    #
    #
    print_title "Checking that Outsiders can not Create Containers"
    begin_verbatim

    mys9s container \
        --create \
        --template=ubuntu \
        --servers=$CONTAINER_SERVER \
        --cmon-user="grumio" \
        --password="p" \
        --log --debug \
        "$container_name"
        
    #$LOG_OPTION $DEBUG_OPTION \
   
    retcode=$?

    if [ "$retcode" -eq 0 ]; then
        warning "Outsiders should not be able to create a container."
        mys9s container --delete "$container_name" --cmon-user=grumio --password=p --log
    else
        success "  o Outsider can't create a container, ok."
    fi

    end_verbatim
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createContainer()
{
    local owner
    local container_name="ft_containers_lxc_00_$$"
    local template

    print_title "Creating Container from Template"
    begin_verbatim

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --template=ubuntu \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name"
    mys9s container --list --long

    #
    # Checking the ip and the owner.
    #
    CONTAINER_IP=$(get_container_ip "$container_name")
    if [ -z "$CONTAINER_IP" ]; then
        failure "The container was not created or got no IP."
        s9s container --list --long
    fi

    if [ "$CONTAINER_IP" == "-" ]; then
        failure "The container got no IP."
        s9s container --list --long
    fi

    owner=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $4}')

    if [ "$owner" != "$USER" ]; then
        failure "The owner of '$container_name' is '$owner', should be '$USER'"
        exit 1
    fi

    end_verbatim

    #
    # Checking if the user can actually log in through ssh.
    #
    print_title "Checking SSH Access"

    begin_verbatim
    if ! is_server_running_ssh "$CONTAINER_IP" "$owner"; then
        failure "User $owner can not log in to $CONTAINER_IP"
    else
        echo "SSH access granted for user '$USER' on $CONTAINER_IP."
    fi

    #
    # Checking the template.
    #
    template=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $3}')

    if [ "$template" != "ubuntu" ]; then
        failure "The template is '$template', should be 'ubuntu'."
    else
        success "  o The template is $template, OK."
    fi

    #
    # We will manipulate this container in other tests.
    #
    LAST_CONTAINER_NAME=$container_name
    end_verbatim
}

#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createAsSystem()
{
    local owner
    local container_name="ft_containers_lxc_01_$$"
    local template

    print_title "Creating Container as System User"
    cat <<EOF
  This test will create a container using the system user when authenticating.
  The owner of the container will be the system user so the various checks has
  to be made by the same user, others won't see this container.

  PLease note that the system user is creating the container, but we also pass
  the --os-user option, so the user $USER will also have an account on the
  created container.

EOF

    begin_verbatim

    #
    # Creating a container.
    #
    mys9s container \
        --create \
        --cmon-user=system \
        --password=secret \
        --os-user="$USER" \
        --os-key-file="/home/$USER/.ssh/id_rsa.pub" \
        --template=ubuntu \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name"
    mys9s container --list --long

    #
    # Checking the ip and the owner.
    #
    CONTAINER_IP=$(get_container_ip "$container_name")
    if [ -z "$CONTAINER_IP" ]; then
        failure "The container was not created or got no IP."
        s9s container --list --long
    else 
        success "  o The container created, ok."
    fi

    if [ "$CONTAINER_IP" == "-" ]; then
        failure "The container got no IP."
        s9s container --list --long
    else
        success "  o The container has the IP $CONTAINER_IP, ok."
    fi

    owner=$(\
        s9s container --list --long --batch \
        --cmon-user=system --password=secret \
        "$container_name" | \
        awk '{print $4}')

    if [ "$owner" != "system" ]; then
        failure "The owner of '$container_name' is '$owner', should be 'system'"
    else
        success "  o The owner is $OWNER, ok."
    fi
   
    #
    # Checking if the user can actually log in through ssh.
    #
    print_title "Checking SSH Access"
    if ! is_server_running_ssh "$CONTAINER_IP" "$USER"; then
        failure "User $USER can not log in to $CONTAINER_IP"
        return 1
    else
        success "  o SSH access granted for user '$USER' on $CONTAINER_IP, ok."
    fi

    #
    # Checking the template.
    #
    template=$(\
        s9s container --list --long --batch \
        --cmon-user=system --password=secret \
        "$container_name" | \
        awk '{print $3}')

    if [ "$template" != "ubuntu" ]; then
        failure "The template is '$template', should be 'ubuntu'"
    else
        success "  o The template is $template, ok."
    fi

    end_verbatim
}

#
# This will try to create some containers with values that should cause failures
# (like duplicate names).
#
function createFail()
{
    local exitCode

    if [ -z "$LAST_CONTAINER_NAME" ]; then
        return 0
    fi

    #
    # Creating a container.
    #
    print_title "Creating Container with Duplicate Name"

    begin_verbatim

    mys9s container \
        --create \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$LAST_CONTAINER_NAME"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with duplicate name should have failed."
    else
        success "  o Container creation failed, ok."
    fi
    
    #
    # Creating a container with invalid provider.
    #
    print_title "Creating Container with Invalid Provider"
    mys9s container \
        --create \
        --cloud="no_such_cloud" \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "node100"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid cloud should have failed."
    else
        success "  o Container creation failed, ok."
    fi
    
    end_verbatim

    #
    # Creating a container with invalid subnet.
    #
    print_title "Creating Container with Invalid Subnet"

    begin_verbatim
    mys9s container \
        --create \
        --subnet-id="no_such_subnet" \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "node101"
    
    exitCode=$?
    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid subnet should have failed."
    else
        success "  o Container creation failed, ok."
    fi

    end_verbatim

    # FIXME: well, this invalid subnet issue is only recognized after the
    # container was created.
    #mys9s container --delete $LOG_OPTION "node101"

    #
    # Creating a container with invalid image.
    #
    print_title "Creating Container with Invalid Image"

    begin_verbatim
    mys9s container \
        --create \
        --image="no_such_image" \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "node102"
    
    exitCode=$?

    if [ "$exitCode" == "0" ]; then
        failure "Creating container with invalid image should have failed."
    else
        success "  o Container creation failed, ok."
    fi

    end_verbatim
}
#
# This will create a container and check if the user can actually log in through
# ssh.
#
function createCentos6()
{
    local container_name="ft_containers_lxc_11_$$"

    #
    # Creating a container Centos 6.
    #
    print_title "Creating Centos 6 Container"
    begin_verbatim

    mys9s container \
        --create \
        --image=centos_6 \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name2"
    check_container "$container_name2"
    end_verbatim
}

function createCentos7()
{
    local container_name="ft_containers_lxc_12_$$"

    #
    # Creating a container Centos 6.
    #
    print_title "Creating Centos 7 Container"
    begin_verbatim

    mys9s container \
        --create \
        --image=centos_7 \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name2"
    check_container "$container_name2"

    end_verbatim
}

function createDebianStretch()
{
    local container_name="ft_containers_lxc_13_$$"
 
    #
    # Creating a container Debian Stretch.
    #
    print_title "Creating Debian Stretch Container"
    begin_verbatim

    mys9s container \
        --create \
        --image=debian_stretch \
        --servers=$CONTAINER_SERVER \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    
    check_exit_code $?
    remember_cmon_container "$container_name4"
    check_container "$container_name4"

    end_verbatim
}

#
# This test will call --stop and then --start on a previously created container.
#
function restartContainer()
{
    local container_name="$LAST_CONTAINER_NAME"
    local status_field

    if [ -z "$container_name" ]; then
        return 0
    fi

    #
    # Stopping the previously created continer. 
    #
    print_title "Stopping Container"
    
    begin_verbatim

    mys9s container --stop \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    check_exit_code $?

    status_field=$(\
        s9s container --list --long --batch "$container_name" \
        | awk '{print $1}')

    if [ "$status_field" != "s" ]; then
        failure "Status is '$status_field' instead of '-'."
    fi
    
    end_verbatim

    #
    # Starting the container we just stopped.
    #
    print_title "Starting Container"
    
    begin_verbatim

    mys9s container --start \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "$container_name"
    check_exit_code $?
    
    status_field=$(\
        s9s container --list --long --batch "$container_name" \
        | awk '{print $1}')
    
    if [ "$status_field" != "u" ]; then
        failure "Status is '$status_field' instead of 'u'."
    fi

    end_verbatim
}

#
# This test will attempt to create a new server (install software and
# everything). For practical reasons we try to do this on the container we just
# created, this way wew are not installing software on a real server.
#
function createServer()
{
    local class
    local retcode

    if [ -z "$CONTAINER_IP" ]; then
        return 0
    fi

    print_title "Creating a Server on a Container"
    
    begin_verbatim

    #
    # Creating a new server, installing software and everything.
    #
    mys9s server \
        --create \
        --servers="lxc://$CONTAINER_IP" \
        $LOG_OPTION \
        $DEBUG_OPTION 

    check_exit_code $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$CONTAINER_IP" | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonLxcServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
    fi

    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_IP/.runtime/state

    end_verbatim

    #
    # Unregistering.
    #
    print_title "Unregistering Server"
    cat <<EOF
  Unregistering the server. First an outsider tries to unregister it, that
  should fail, but then the owner does it and that should secceed.
EOF

    begin_verbatim

    mys9s server \
        --unregister \
        --servers="lxc://$CONTAINER_IP" \
        --cmon-user="grumio" \
        --password="p"

    retcode=$?
    if [ "$retcode" -ne 0 ]; then
        success "  o Outsiders can't unregister the server, ok."
    else
        failure "Outsider should not be able to unregister the server."
    fi

    # And the owner should be able...    
    mys9s server --unregister --servers="lxc://$CONTAINER_IP"

    check_exit_code_no_job $?
    mys9s server --list --long 

    end_verbatim
}

#
# This will try to manipulate a container that does not exist. The jobs should
# fail, the return value should be false.
#
function failOnContainers()
{
    local retcode

    print_title "Trying to Manipulate Unexisting Containers"

    begin_verbatim

    #
    # Trying to delete a container that does not exists.
    #
    mys9s container \
        --delete \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "unexisting_container_$$"

    retcode=$?
    if [ $retcode -eq 0 ]; then
        failure "Reporting success while deleting unexsiting container."
    else
        success "  o Command failed, ok."
    fi
    
    #
    # Trying to stop a container that does not exists.
    #
    mys9s container \
        --stop \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "unexisting_container_$$"

    retcode=$?
    if [ $retcode -eq 0 ]; then
        failure "Reporting success while stopping unexsiting container."
    else
        success "  o Command failed, ok."
    fi
    
    #
    # Trying to start a container that does not exists.
    #
    mys9s container \
        --start \
        $LOG_OPTION \
        $DEBUG_OPTION \
        "unexisting_container_$$"

    retcode=$?
    if [ $retcode -eq 0 ]; then
        failure "Reporting success while starting unexsiting container."
    else
        success "  o Command failed, ok."
    fi

    end_verbatim
}

#
# This will delete the containers we created before.
#
function deleteContainer()
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
            $DEBUG_OPTION \
            "$container"
    
        check_exit_code $?
    done

    #mys9s job --list
    end_verbatim
}


#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    if [ "$1" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateOutsider
        runFunctionalTest registerServer
        runFunctionalTest checkServerAccess
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateOutsider
    runFunctionalTest registerServer
    runFunctionalTest checkServer
    runFunctionalTest checkServerAccess

    runFunctionalTest createContainer
    runFunctionalTest createAsSystem
    runFunctionalTest createFail

    #runFunctionalTest createCentos6
    runFunctionalTest createCentos7
    runFunctionalTest createDebianStretch

    runFunctionalTest restartContainer
    runFunctionalTest createServer
    runFunctionalTest failOnContainers
    runFunctionalTest deleteContainer
fi

endTests
