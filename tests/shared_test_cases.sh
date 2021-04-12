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

    begin_verbatim
    node_ip=$(create_node --autodestroy --template "ubuntu-s9s" "$node_name") 
    SECONDARY_CONTROLLER_IP="$node_ip"
    SECONDARY_CONTROLLER_URL="https://$SECONDARY_CONTROLLER_IP:9556"
    end_verbatim


    print_title "Pulling Source on $SECONDARY_CONTROLLER_IP"
    begin_verbatim
    ssh_to_controller "cd $sdir && git pull"
    if [ $? -ne 0 ]; then
        failure "Failed to git pull."
        return 1
    fi
    end_verbatim

    print_title "Installing Packages on $SECONDARY_CONTROLLER_IP"
    begin_verbatim
    ssh_to_controller "sudo apt install -y pkg-config"
    ssh_to_controller "sudo apt install -y libldap2-dev"
    ssh_to_controller "sudo apt install -y libhiredis-dev"
    end_verbatim

    print_title "Running autogen.sh on $SECONDARY_CONTROLLER_IP"
    begin_verbatim
    ssh_to_controller "cd $sdir && env | grep PATH"
    ssh_to_controller "cd $sdir && ./autogen.sh >/dev/null"
    if [ $? -ne 0 ]; then
        failure "Failed to configure."
        return 1
    fi
    end_verbatim

    print_title "Compiling Source"
    begin_verbatim
    ssh_to_controller "cd $sdir && make -j15 >/dev/null"
    if [ $? -ne 0 ]; then
        failure "Failed to compile."
        return 1
    fi
    end_verbatim

    ssh_to_controller "rm -rvf /var/tmp/cmon* /tmp/cmon*"

    #
    # Starting the secondary controller.
    #
    print_title "Starting Secondary Controller"
    begin_verbatim
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
    end_verbatim

    print_title "Testing Connection to the New Controller"
    begin_verbatim
    mys9s user \
        --list --long \
        --controller=$SECONDARY_CONTROLLER_URL \
        --cmon-user=system \
        --password=secret
    
    check_exit_code_no_job $?
    end_verbatim

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

    begin_verbatim
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
    end_verbatim
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

#
# This test will add a proxy sql node.
#
function testAddRemoveProxySql()
{
    local node_name="${MYBASENAME}proxysql$$"
    local node
    local numberOfNodes

    print_title "Adding and Removing a ProxySQL Node"
    cat <<EOF | paragraph
  This test will add a ProxySQL node to the previously created cluster. Then the
  test will remove the ProxySQL node. It seems removing is only possible when
  the --force option is provided, because the controller can not stop the
  proxysql process for some reason.

EOF

    begin_verbatim
    check_number_of_proxysql_nodes 0

    node=$(create_node --autodestroy "$node_name")

    #
    # Adding a node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="proxySql://$node:6032" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?

    mysleep 15
    check_number_of_proxysql_nodes 1

    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="proxySql://$node:6032" \
        --force \
        $LOG_OPTION \
        $DEBUG_OPTION

    check_exit_code $?
    check_number_of_proxysql_nodes 0

    end_verbatim
}

#
# This test will first add a HaProxy node, then remove from the cluster. The
# idea behind this test is that the remove-node call should be identify the node
# using the IP address if there are multiple nodes with the same IP (one galera
# node and one haproxy node on the same host this time).
#
function testAddRemoveHaProxy()
{
    local node_name="${MYBASENAME}haproxy$$"
    local node
    
    print_title "Adding and Removing HaProxy node"
    cat <<EOF | paragraph
  This test will add and then remove a HaProxy node.
EOF

    begin_verbatim
   
    check_number_of_haproxy_nodes 0
    node=$(create_node --autodestroy "$node_name")

    #
    # Adding a HaProxy node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="haProxy://$node:9600" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    mysleep 15

    check_number_of_haproxy_nodes 1

    #
    # Remove a HaProxy node from the cluster.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$node:9600" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    
    check_number_of_haproxy_nodes 0

    end_verbatim
}

function testAddRemoveMaxScale()
{
    local node_name="${MYBASENAME}maxscale$$"
    local node
    
    print_title "Adding and Removing MaxScale node"
    cat <<EOF | paragraph
  This test will add and then remove a MaxScale node. The test checks that the
  jobs to add and remove are successfully finished and if the loadbalancer node
  appears in and disappears from the node list.
EOF

    begin_verbatim
    check_number_of_maxscale_nodes 0

    node=$(create_node --autodestroy "$node_name")

    #
    # Adding a MaxScale node to the cluster.
    #
    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="maxScale://$node:6603" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    mysleep 15

    check_number_of_maxscale_nodes 1

    #
    # Remove a MaxScale node from the cluster.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$node:6603" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    
    check_number_of_maxscale_nodes 0

    end_verbatim
}

# This is not yet funcitonal...
function testAddRemoveKeepalived()
{
    local node_name="${MYBASENAME}_keepalived_$$"
    local node
    
    #
    #
    #
    print_title "Adding and Removing Keepalived node"

    if [ "$OPTION_NUMBER_OF_NODES" -lt 2 ]; then
        cat <<EOF | paragraph
  Keepalived needs at least two data nodes, skipping this test.
EOF
        return 0
    fi


    begin_verbatim
    
    node=$(create_node --autodestroy "$node_name")

    #
    # Adding a Keepalived node to the cluster.
    #
    mys9s node --list --long

    mys9s cluster \
        --add-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="keepalived://$node:6603" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    
    check_exit_code $?
    mysleep 15

    mys9s node --list --long
    
    #
    # Remove a Keepalived node from the cluster.
    #
    mys9s cluster \
        --remove-node \
        --cluster-id=$CLUSTER_ID \
        --nodes="$node:6603" \
        $LOG_OPTION \
        $DEBUG_OPTION
    
    check_exit_code $?
    
    mys9s node --list --long --color=always

    end_verbatim
}



