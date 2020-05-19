#
# This will register the container server. 
#
function registerServer()
{
    local class

    print_title "Registering Container Server"
    begin_verbatim

    #
    # Registering a container server.
    #
    mys9s server \
        --register \
        --servers="lxc://$CONTAINER_SERVER" 

    check_exit_code_no_job $?
    end_verbatim    

    #
    # Checking the state and the class name... 
    #
    check_container_server \
        --class        CmonLxcServer \
        --server-name  "$CONTAINER_SERVER" \
        --cloud        "lxc" \
        --class       "CmonLxcServer"

    mys9s tree --cat /$CONTAINER_SERVER/.runtime/state

}

