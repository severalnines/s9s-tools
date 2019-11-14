#
# Pings the controller to check if it is up.
#
function testPing()
{
    print_title "Pinging The Controller"

    begin_verbatim

    #
    # Pinging. 
    #
    mys9s cluster --ping --cluster-id=0 --cmon-user=system --password=secret

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
    else
        success "  o Cluster replied to ping, ok."
    fi

    end_verbatim
}

#
# Creates a secondary controller on which we can test the controller change
# use-case.
#
function createController()
{
    local node_name="ft_clustersave_00_$$"
    local sdir="~/clustercontrol-enterprise"
    local tdir="$sdir/tests"

    print_title "Creating Secondary Controller"
    cat <<EOF
Creating a secondary controller in a container. For this we need to pull and
compile the controller source. This may take some time.

EOF

    node_ip=$(create_node --autodestroy --template "ubuntu-s9s" "$node_name") 
    SECONDARY_CONTROLLER_IP="$node_ip"
    SECONDARY_CONTROLLER_URL="https://$SECONDARY_CONTROLLER_IP:9556"

    print_title "Pulling Source on $SECONDARY_CONTROLLER_IP"
    ssh_to_controller "cd $sdir && git pull"
    if [ $? -ne 0 ]; then
        failure "Failed to git pull."
        return 1
    fi
    
    print_title "Installing Packages on $SECONDARY_CONTROLLER_IP"
    ssh_to_controller "sudo apt install pkg-config"
    
    print_title "Running autogen.sh on $SECONDARY_CONTROLLER_IP"
    ssh_to_controller "cd $sdir && env | grep PATH"
    ssh_to_controller "cd $sdir && ./autogen.sh >/dev/null"
    if [ $? -ne 0 ]; then
        failure "Failed to configure."
        while true; do 
            sleep 1000
        done

        return 1
    fi

    print_title "Compiling Source"
    ssh_to_controller "cd $sdir && make -j15 >/dev/null"
    if [ $? -ne 0 ]; then
        failure "Failed to compile."
        return 1
    fi
    
    ssh_to_controller "rm -rvf /var/tmp/cmon* /tmp/cmon*"

    #
    # Starting the secondary controller.
    #
    print_title "Starting Secondary Controller"
    rm -f nohup.out

    nohup ssh \
        -o ConnectTimeout=1 \
        -o UserKnownHostsFile=/dev/null \
        -o StrictHostKeyChecking=no \
        -o LogLevel=quiet \
        $SECONDARY_CONTROLLER_IP \
        -- ~/clustercontrol-enterprise/tests/runftfull.sh &
    
    SSH_PID=$!

    if [ $? -ne 0 ]; then
        failure "Failed to start controller."
        return 1
    fi

    for n in $(seq 1 60); do
        if grep --quiet testWait "nohup.out" 2>/dev/null; then
            break
        fi
        sleep 1
    done

    cat "nohup.out"
    
    print_title "Testing Connection to the New Controller"
    mys9s user \
        --list --long \
        --controller=$SECONDARY_CONTROLLER_URL \
        --cmon-user=system \
        --password=secret
    
    check_exit_code_no_job $?

    return 0
    while true; do
        sleep 10
        echo -n "."
    done
}

#
# This is where we create "grumio" an outsider that is not a superuser and not
# related to the test users we have. So this "grumio" user should not have
# access to many of the objects the test user creates.
#
function testCreateOutsider()
{
    print_title "Creating an 'outsider' User"
    cat <<EOF
  This test will create a usr called 'grumio' who is an outsider, used in the
  test to check situations where the user should not have access to various
  objects.

EOF

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --group="plebs" \
        --create-group \
        --generate-key \
        --first-name="Grumio" \
        --email-address="grumio@rome.com" \
        --new-password="p" \
        grumio
    
    check_exit_code_no_job $?
}

function testRegisterLxcServer()
{
    local class

    print_title "Registering Container Server"
    cat <<EOF | paragraph
  Here we register an LXC server that will be used to create containers. No
  software will be installed, just the registration is done here, the server
  should be fully prepared to run LXC containers already.

EOF

    begin_verbatim

    #
    # Creating a container.
    #
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?

    mys9s server --stat "$CONTAINER_SERVER"
    check_exit_code_no_job $?

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$CONTAINER_SERVER" \
        | grep "Class:" | awk '{print $2}')

    if [ "$class" != "CmonLxcServer" ]; then
        failure "Created server has a '$class' class and not 'CmonLxcServer'."
    else
        success "  o The container server has class $class, ok."
    fi
    
    #
    # Checking the state... TBD
    #
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state

    end_verbatim
}

#
# This will delete the containers we created before.
#
function testDeleteTestContainers()
{
    local containers=$(cmon_container_list)
    local container

    print_title "Deleting Containers Created by the Test"

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

    end_verbatim
}


#
# This will register the container server. 
#
function testRegisterServer()
{
    local class

    print_title "Registering Container Server"

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
}
