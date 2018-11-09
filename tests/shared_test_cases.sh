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
    
    print_title "Running autogen.sh on $SECONDARY_CONTROLLER_IP"
    ssh_to_controller "cd $sdir && env | grep PATH"
    ssh_to_controller "cd $sdir && ./autogen.sh"
    if [ $? -ne 0 ]; then
        failure "Failed to configure."
        while true; do 
            sleep 1000
        done

        return 1
    fi

    print_title "Compiling Source"
    ssh_to_controller "cd $sdir && make -j15"
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


