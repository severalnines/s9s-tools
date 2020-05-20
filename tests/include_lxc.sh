#
# This will register the container server. 
#
function registerServer()
{
    local class

    print_title "Registering Container Server"
    cat <<EOF | paragraph
  This is the registerServer test from the include_lxc.sh file that will
  register an LXC server and check if it is indeed registered. We do this
  in tests where the controller itself needs to create conatiners.
EOF

    begin_verbatim

    #
    # Registering a container server.
    #
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?
    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state

    end_verbatim    

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonLxcServer \
        --server-name  "$CONTAINER_SERVER" \
        --cloud        "lxc" \
        --class       "CmonLxcServer"


}

