#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
OPTION_INSTALL=""
CLUSTER_NAME="galera_001"

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

SUPPORTED TESTS:
  o testCreateUser       Creates a user with normal user privileges.
  o testCreateSuperuser  Creates a user with superuser privileges.



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
    print_title "Creating a Superuser"
    cat <<EOF
This test will create a user with superuser privileges under the name
"superuser" and checks if the exit code is 0.

EOF

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
    #mys9s tree --list --long
}

function testLicenseDevice()
{
    local retCode
    print_title "Checking Access to the License Device File"

    mys9s tree \
        --cat \
        /.runtime/cmon_license 
    
    retCode=$?
    if [ $retCode -eq 0 ]; then
        failure "Normal user should not have read access to the license file."
    else
        success "  o Normal user can't read the license, ok."
    fi

    mys9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/cmon_license 
    
    retCode=$?
    if [ $retCode -eq 0 ]; then
        success "  o Superuser can read the license file, ok."
    else
        failure "Superuser should be able to read the license."
    fi

    print_title "Uploading License"
    cat <<EOF
  This test will try to upload a license to the controller. Both valid and
  invalid attempts are tested. Here is an example how we do that:

    echo "\$license" | \\
        s9s tree \\
            --save \\
            --cmon-user=system \\
            --password=secret \\
            "/.runtime/cmon_license"

EOF

    # 
    # Trying with an invalid license. 
    #
    license=$(cat <<EOF 
wogICAgImNvbXBhbnkiOiAib25lIiwKICAgICJlbWFpbF9hZGRyZXNzIjogInR3byIsCiAgICAi
ZXhwaXJhdGlvbl9kYXRlIjogIjIwMjEtMTItMzFUMDA6MDA6MDAuMDAxWiIsCiAgICAibGljZW5z
ZWRfbm9kZXMiOiAwLAogICAgInR5cGUiOiAiRGVtbyIKfQp7czlzLXNpZ25hdHVyZS1zZXBhcmF0
b3J9CkYoFPzdyxqj55jO2GMANgYs/YgcSyxvYCcTsGZuj6P+pKMtE8gZbGCifdc2D3Y5pcP6YfKh
Om44ElVqMbYe9oeiyxZbUvmxZJYTsEX1KTnoaRys2dGQmfixehRIHlvcE9QT/ALoqiSOqkGTdT2G
Y+pklXhSuGGy+UxWPAeheZ/Gb27rE+VJcouCNXpBeE478ekdnR7ExlPTOX0YBafm3pukcMZ8ddkF
9zBYXocy3+YkPdRYz6brXvGuuspCW7FGubxNlywOdEofP4R1qD6pdLVnoH1dgmWLlT0CJVeMhgDF
fLAVrLNNedQ69sAv5JyDmiyrf/T7an3iiSZWHlH31vwY8LdhRQiiycNJ6BRGBIsuPN8Joi4GQosk
N1OAth0O8lm5SR82YgfrpQB6HFyre1qMQqj6kuHJ4hInPer7BeSLMUr+1XiOU0NKjS7HwVdveDvy
D6/KcALlH7ifkU89U6O6qR6NF+T5dQJfccnRw1u93lIhItdc3pH+o3+BNcRX1mMmBM1ysDDpVYHY
7uAov5OmDajW37DXgeNkJYhdYeviPaoXQYeiMJmDaJ0BDqEDPB5AcdFBRLZ3FgdoTJUyRsw6iYck
0ZXMgNVbL8bNf07K9ooEjByY8Mia9HNCFo8XXEjPQqUVdet3j/KkecoJB8kLXhjsFEvGxaR601+Z
/JfV
EOF
)
    echo "$license" | \
        s9s tree \
            --save \
            --cmon-user=system \
            --password=secret \
            /.runtime/cmon_license
    
    retCode=$?
    if [ $retCode -eq 0 ]; then
        failure "Invalid license should not be accepted."
    else
        success "  o Invalid license is rejected, ok."
    fi
    
    #
    # Trying with a valid one, but normal user.
    #
    license=$(cat <<EOF 
ewogICAgImNvbXBhbnkiOiAib25lIiwKICAgICJlbWFpbF9hZGRyZXNzIjogInR3byIsCiAgICAi
ZXhwaXJhdGlvbl9kYXRlIjogIjIwMjEtMTItMzFUMDA6MDA6MDAuMDAxWiIsCiAgICAibGljZW5z
ZWRfbm9kZXMiOiAwLAogICAgInR5cGUiOiAiRGVtbyIKfQp7czlzLXNpZ25hdHVyZS1zZXBhcmF0
b3J9CkYoFPzdyxqj55jO2GMANgYs/YgcSyxvYCcTsGZuj6P+pKMtE8gZbGCifdc2D3Y5pcP6YfKh
Om44ElVqMbYe9oeiyxZbUvmxZJYTsEX1KTnoaRys2dGQmfixehRIHlvcE9QT/ALoqiSOqkGTdT2G
Y+pklXhSuGGy+UxWPAeheZ/Gb27rE+VJcouCNXpBeE478ekdnR7ExlPTOX0YBafm3pukcMZ8ddkF
9zBYXocy3+YkPdRYz6brXvGuuspCW7FGubxNlywOdEofP4R1qD6pdLVnoH1dgmWLlT0CJVeMhgDF
fLAVrLNNedQ69sAv5JyDmiyrf/T7an3iiSZWHlH31vwY8LdhRQiiycNJ6BRGBIsuPN8Joi4GQosk
N1OAth0O8lm5SR82YgfrpQB6HFyre1qMQqj6kuHJ4hInPer7BeSLMUr+1XiOU0NKjS7HwVdveDvy
D6/KcALlH7ifkU89U6O6qR6NF+T5dQJfccnRw1u93lIhItdc3pH+o3+BNcRX1mMmBM1ysDDpVYHY
7uAov5OmDajW37DXgeNkJYhdYeviPaoXQYeiMJmDaJ0BDqEDPB5AcdFBRLZ3FgdoTJUyRsw6iYck
0ZXMgNVbL8bNf07K9ooEjByY8Mia9HNCFo8XXEjPQqUVdet3j/KkecoJB8kLXhjsFEvGxaR601+Z
/JfV
EOF
)
    echo "$license" | \
        s9s tree \
            --save \
            /.runtime/cmon_license
    
    retCode=$?
    if [ $retCode -eq 0 ]; then
        failure "Normal user should not be able to upload license."
    else
        success "  o Normal user can't set license, ok."
    fi
   
    #
    # Same, valid license, but superuser this time. 
    #
    echo "$license" | \
        s9s tree \
            --save \
            --cmon-user=system --password=secret \
            /.runtime/cmon_license

    retCode=$?
    if [ $retCode -eq 0 ]; then
        success "  o Superuser can read the license file, ok."
    else
        failure "Superuser should be able to read the license."
    fi
   
    #
    #
    #
    mys9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/cmon_license 

    json=$(mys9s tree --cat \
        --cmon-user=system --password=secret \
        /.runtime/cmon_license)

    if $(echo "$json" | grep -q one); then
        success "  o The license seems to be updated, ok."
    else
        failure "The license seems to be not updated."
    fi
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

    print_title "Registering a Container Server"
    cat <<EOF
This test will register a container server and check if it properly shows in the
tree.

EOF

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
        #echo "  checking line: $line"
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
    cat <<EOF
This test will create a container outside of the Cmon Controller (manually, 
by executing a few commands on the server) then it will check if the container
appears in the list the controller maintains about the containers.
EOF

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
   
    #
    #
    #
    mys9s tree --list --long $CONTAINER_SERVER/containers
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch $CONTAINER_SERVER/containers)
    do
        #echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case $name in 
            $container_name)
                if [ "$owner" != "pipas" ]; then
                    failure "Owner is '$owner'."
                else
                    success "  o owner is '$owner', ok"
                fi

                if [ "$group" != "users" ]; then
                    failure "Group is '$group'."
                else
                    success "  o group is '$group', ok"
                fi

                if [ "$mode"  != "crwxrw----" ]; then
                    failure "Mode is '$mode'." 
                else
                    success "  o mode is '$mode', ok" 
                fi

                object_found="yes"
                ;;
        esac
    done
    IFS=$old_ifs    

    if [ -z "$object_found" ]; then
        failure "Object was not found."
    else
        success "  o the new container is found, ok"
    fi
}

#####
# Creating a Galera cluster.
#
function testCreateCluster()
{
    local old_ifs="$IFS"
    local line
    local name
    local mode
    local owner
    local group

    #
    # Creating the cluster...
    #
    print_title "Creating a Cluster"
    cat <<EOF 
  In this test we create a cluster and check various properties of it (e.g. the
  owner and the group owner). 

EOF
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$CONTAINER_IP" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=5.6 \
        --wait

    check_exit_code $?
    
    #
    # Checking the access rights of the cluster.
    #
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch "$CLUSTER_NAME"); do
        #echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        if [ "$owner" != "pipas" ]; then
            failure "Owner of '$name' is '$owner'."
        else
            success "  o owner of '$name' is '$owner', ok"
        fi

        if [ "$group" != "users" ]; then
            failure "Group is '$group'."
        else
            success "  o group of '$name' is '$group', ok"
        fi
    done
    IFS=$old_ifs    
}

#####
# Creating databases.
#
function testCreateDatabase()
{
    local old_ifs="$IFS"
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
    cat <<EOF
This test will create a few databases then it checks that the databases will
appear in the tree with the right properties.

EOF

    mys9s cluster \
        --create-database \
        --cluster-name="$CLUSTER_NAME" \
        --db-name="domain_names_ngtlds_diff" \
        --batch

    check_exit_code_no_job $?

    mys9s cluster \
        --create-database \
        --cluster-name="$CLUSTER_NAME" \
        --db-name="domain_names_diff" \
        --batch
    
    check_exit_code_no_job $?

    mys9s cluster \
        --create-database \
        --cluster-name="$CLUSTER_NAME" \
        --db-name="whois_records_delta" \
        --batch
    
    check_exit_code_no_job $?

    # FIXME: We wait here, we should not need to.
    sleep 15

    mys9s tree --list --long "$CLUSTER_NAME/databases"
    
    IFS=$'\n'
    for line in $(s9s tree --list --long --batch "$CLUSTER_NAME/databases")
    do
        #echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case "$name" in 
            domain_names_diff)
                success "  o database $name found, ok"
                if [ "$owner" != "pipas" ]; then
                    failure "Owner is '$owner'."
                else
                    success "  o owner is $owner, ok"
                fi
                
                if [ "$group" != "users" ]; then
                    failure "Group is '$group'."
                else
                    success "  o group is $group, ok"
                fi

                if [ "$mode"  != "brwxrw----" ]; then
                    failure "Mode is '$mode'." 
                else
                    success "  o mode is $mode, ok"
                fi

                let n_object_found+=1
                ;;

            domain_names_ngtlds_diff)
                success "  o database $name found, ok"
                if [ "$owner" != "pipas" ]; then
                    failure "Owner is '$owner'."
                else
                    success "  o owner is $owner, ok"
                fi
                
                if [ "$group" != "users" ]; then
                    failure "Group is '$group'."
                else
                    success "  o group is $group, ok"
                fi

                if [ "$mode"  != "brwxrw----" ]; then
                    failure "Mode is '$mode'." 
                else
                    success "  o mode is $mode, ok"
                fi

                let n_object_found+=1
                ;;

            whois_records_delta)
                success "  o database $name found, ok"
                if [ "$owner" != "pipas" ]; then
                    failure "Owner is '$owner'."
                else
                    success "  o owner is $owner, ok"
                fi
                
                if [ "$group" != "users" ]; then
                    failure "Group is '$group'."
                else
                    success "  o group is $group, ok"
                fi
                
                if [ "$mode"  != "brwxrw----" ]; then
                    failure "Mode is '$mode'." 
                else
                    success "  o mode is $mode, ok"
                fi

                let n_object_found+=1
                ;;

            *)
                failure "Database '$name' is unexpected."
        esac
    done
    IFS=$old_ifs    

    if [ "$n_object_found" -lt 3 ]; then
        failure "Some databases were not found."
    else
        success "  o all $n_objects_found databases found, ok"
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
    print_title "Moving Objects into Subfolder"
    cat <<EOF
In this scenario the user moves some objects into a subfolder, then moves the
user object itself. All the objects should remain visible in a chroot
environment.

EOF

    mys9s tree --mkdir "$TEST_PATH"
    mys9s tree --move "/$CONTAINER_SERVER" "$TEST_PATH"
    mys9s tree --move "/$CLUSTER_NAME" "$TEST_PATH"
    mys9s tree --move "/pipas" "$TEST_PATH"

    mys9s tree --tree 
    
    #
    #
    #
    mys9s tree --list --long --recursive --full-path

    IFS=$'\n'
    for line in $(s9s tree --list --long --recursive --full-path --batch)
    do
        #echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        line=$(echo "$line" | sed 's/3, 1/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        case "$name" in 
            /$CLUSTER_NAME)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "crwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /$CLUSTER_NAME/databases/domain_names_diff)
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

    #echo "n_object_found: $n_object_found"
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
        #echo "  checking line: $line"
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        line=$(echo "$line" | sed 's/3, 1/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')
        case "$name" in
            /home/pipas/pipas)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "admins" ] && failure "Group is '$group'."
                [ "$mode"  != "urwxr--r--" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /home/pipas/$CLUSTER_NAME)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "crwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /home/pipas/$CONTAINER_SERVER)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "srwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /home/pipas/$CLUSTER_NAME/databases/domain_names_diff)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrw----" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;
        esac
    done
    IFS=$old_ifs    
    
    if [ "$n_object_found" -lt 4 ]; then
        failure "Some objects were not found."
    else
        success "  o all objects were found"
    fi
}

function testStats()
{
    print_title "Checking CDT Path of Objects"

    mys9s user --stat "$USER"
    
    mys9s server --stat
    mys9s server --list --long
    
    mys9s cluster --stat

    mys9s server --list --long
}

#
# Checking the --get-acl in a chroot environment.
#
function testAclChroot()
{
    print_title "Checking getAcl replies"

    THE_NAME=$(s9s tree --get-acl --print-json / | jq .object_name)
    THE_PATH=$(s9s tree --get-acl --print-json / | jq .object_path)
    if [ "$THE_PATH" != '"/"' ]; then
        s9s tree --get-acl --print-json / | jq .
        failure "The path should be '/' in getAcl reply not '$THE_PATH'."
    else
        success "  o path is '$THE_PATH', ok"
    fi

    if [ "$THE_NAME" != '""' ]; then
        s9s tree --get-acl --print-json / | jq .
        failure "The object name should be empty in getAcl reply."
    else
        success "The name is '$THE_NAME', ok"
    fi

    THE_NAME=$(s9s tree --get-acl --print-json /galera_001 | jq .object_name)
    THE_PATH=$(s9s tree --get-acl --print-json /galera_001 | jq .object_path)
    if [ "$THE_PATH" != '"/"' ]; then
        s9s tree --get-acl --print-json /galera_001 | jq .
        failure "The path should be '/' in getAcl reply, not '$THE_PATH'."
    else
        success "  o path is '$THE_PATH', ok"
    fi

    if [ "$THE_NAME" != '"galera_001"' ]; then
        s9s tree --get-acl --print-json /galera_001 | jq .
        failure "The object name should be 'galera_001' in getAcl reply."
    else
        success "The name is '$THE_NAME', ok"
    fi

    THE_NAME=$(s9s tree --get-acl --print-json /galera_001/databases | jq .object_name)
    THE_PATH=$(s9s tree --get-acl --print-json /galera_001/databases | jq .object_path)
    if [ "$THE_PATH" != '"/galera_001"' ]; then
        s9s tree --get-acl --print-json /galera_001/databases | jq .
        failure "The path should be '/galera_001' in getAcl reply."
    else
        success "  o path is '$THE_PATH', ok"
    fi

    if [ "$THE_NAME" != '"databases"' ]; then
        s9s tree --get-acl --print-json /galera_001/databases | jq .
        failure "The object name should be 'databases' in getAcl reply."
    else
        success "The name is '$THE_NAME', ok"
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

    check_exit_code_no_job $?
}

#####
# Adding an ACL.
#
function testManipulate()
{
    print_title "Adding an ACL Entry"

    mys9s tree --add-acl --acl="user:pipas:rwx" /tmp
    check_exit_code_no_job $?

    #
    # Changing the owner
    #
    print_title "Changing the Owner"
    mys9s tree --chown --owner=admin:admins /tmp
    check_exit_code_no_job $?

    mys9s tree --list --directory --long /tmp
    OWNER=$(s9s tree --list --directory --batch --long /tmp | awk '{print $3}')
    GROUP=$(s9s tree --list --directory --batch --long /tmp | awk '{print $4}')
    if [ "$OWNER" != 'admin' ]; then
        failure "The owner should be 'admin' not '$OWNER'."
    else 
        success "  o the owner is $OWNER, ok"
    fi

    if [ "$GROUP" != 'admins' ]; then
        failure "The group should be 'admins' not '$GROUP'."
    else
        success "  o the group is $GROUP, ok"
    fi
}

#####
# A final view and exit.
#
function testTree()
{
    local name
    local n_object_found=0

    print_title "Printing and Checking Tree"

    mys9s tree --list 
    mys9s tree --list --long

    for name in $(s9s tree --list); do
        case "$name" in
            $CLUSTER_NAME)
                let n_object_found+=1
                ;;

            $CONTAINER_SERVER)
                let n_object_found+=1
                ;;

            tmp)
                let n_object_found+=1
                ;;

            pipas)
                let n_object_found+=1
                ;;

        esac
    done
    
    if [ "$n_object_found" -lt 4 ]; then
        failure "Some objects were not found."
    fi
}

function testMoveBack()
{
    print_title "Moving Objects from Chroot"
    cat <<EOF
Here the superuser moves the objects back into the root directory out from the
chroot environment of the other user. The other user should not see the 
objects.
EOF
    mys9s tree --tree --all --cmon-user=system --password=secret

    mys9s tree \
        --cmon-user=system \
        --password=secret \
        --move "$TEST_PATH/$CONTAINER_SERVER" "/"

    check_exit_code_no_job $?
 
    mys9s tree \
        --cmon-user=system \
        --password=secret \
        --move "$TEST_PATH/$CLUSTER_NAME" "/"

    check_exit_code_no_job $?

    mys9s tree --tree --all --cmon-user=system --password=secret
    mys9s tree --tree --all
   
    #
    # Checking if the superuser see the cluster while the user do not (because
    # the user is in a chroot environment).
    #
    if $(s9s cluster --list --long --cmon-user=system --password=secret | grep -q "$CLUSTER_NAME"); then
        success "  o the superuser sees the cluster, ok"
    else
        failure "The superuser does not see the cluster."
        mys9s cluster --list --long --cmon-user=system --password=secret
    fi

    if $(s9s cluster --list --long | grep -q "$CLUSTER_NAME"); then
        failure "The user should not see the cluster."
        mys9s cluster --list --long
    else
        success "  o the user does not see the cluster, ok"
    fi

    #
    # Checking the visibility of the nodes.
    #
    if $(s9s node --list --long | grep -q "$CLUSTER_NAME"); then
        failure "The user should not see the nodes."
        mys9s node --list --long 
    else
        success "  o the user does not see the nodes, ok"
    fi

    if $(s9s node --list --long --cmon-user=system --password=secret | grep -q "$CLUSTER_NAME"); then
        success "  o the nodes are visible for the system user, ok"
    else
        failure "The nodes should be visible for the system user."
        mys9s node --list --long --cmon-user=system --password=secret
    fi

    #
    # Checking the visibility of the server.
    #
    if $(s9s server --list --long | grep -q "$CONTAINER_SERVER"); then
        failure "The user should not see the server."
        mys9s server --list --long 
    else
        success "  o the user does not see the server, ok"
    fi

    if $(s9s server --list --long --cmon-user=system --password=secret | grep -q "$CONTAINER_SERVER"); then
        success "  o the server is visible for the system user, ok"
    else
        failure "The server should be visible for the system user."
        mys9s server --list --long --cmon-user=system --password=secret
    fi

    #mys9s server --list --long
    #mys9s server --list --long --cmon-user=system --password=secret
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
        runFunctionalTest testCreateUser
        runFunctionalTest testCreateSuperuser
        runFunctionalTest testLicenseDevice
        runFunctionalTest testRegisterServer
        runFunctionalTest testCreateContainer
        runFunctionalTest testCreateCluster
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateUser
    runFunctionalTest testCreateSuperuser
    runFunctionalTest testLicenseDevice
    runFunctionalTest testRegisterServer
    runFunctionalTest testCreateContainer
    runFunctionalTest testCreateCluster
    runFunctionalTest testCreateDatabase
    runFunctionalTest testCreateAccount
    runFunctionalTest testMoveObjects
    runFunctionalTest testStats
    runFunctionalTest testAclChroot
    runFunctionalTest testCreateFolder
    runFunctionalTest testManipulate
    runFunctionalTest testTree
    runFunctionalTest testMoveBack
    runFunctionalTest deleteContainers
fi

endTests

