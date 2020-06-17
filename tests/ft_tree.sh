#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
OPTION_INSTALL=""
CLUSTER_NAME="galera_001"

export S9S_USER_CONFIG="$HOME/.s9s/$MYBASENAME.conf"

cd $MYDIR
source include.sh
source shared_test_cases.sh

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
  o testCreateOutsider   Creates an outsider user who should not have access.
  o testLicense          Reading the license through CDT.
  o testLicenseUpload    Uploading the license through CDT.
  o testRegisterServer   Registers a container server.
  o testCreateContainer  Creates a container.
  o testCreateCluster    Creates a cluster on the container.
  o testPingAccess       Tests if outsiders can not ping a cluster.
  o testJobAccess        Checks if outsiders can not create a job.
  o testLogAccess        Checks if outsiders can not see the logs of a cluster.
  o testCreateDatabase   Creates some databases.
  o testCreateAccount    Creates some database accounts.
  o testMoveObjects      Moves objects in the tree.
  o testStats
  o testAclChroot
  o testCreateFolder
  o testAddAcl           Adds an ACL entry to a CDT entry.
  o testChangeOwner      Changes the owner of a CDT entry.
  o testTree
  o testMoveBack
  o deleteContainers

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

    #
    #
    #
    print_title "Creating a Normal User"

    begin_verbatim
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

    end_verbatim
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

    begin_verbatim
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
    end_verbatim
}

function testLicense()
{
    local retCode

    #
    #
    #
    print_title "Checking Access to the License Device File"

    begin_verbatim
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

    end_verbatim
}

function testLicenseUpload()
{
    local retCode

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

    begin_verbatim

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

    end_verbatim
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

    begin_verbatim
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
                [ "$mode"  != "srwxrwx---" ] && failure "Mode is '$mode'." 
                object_found="yes"
                ;;
        esac
    done
    IFS=$old_ifs    

    if [ -z "$object_found" ]; then
        failure "Object was not found."
    fi

    end_verbatim
}

#
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
  Creating a container through the Cmon Contoller, checking that everything is
  ok and the container is visible through the controller.

EOF

    begin_verbatim

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

                if [ "$mode"  != "crwxrwx---" ]; then
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

    end_verbatim
}

#
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

    begin_verbatim
    mys9s cluster \
        --create \
        --cluster-type=galera \
        --nodes="$CONTAINER_IP" \
        --vendor=percona \
        --cluster-name="$CLUSTER_NAME" \
        --provider-version=5.6 \
        --cmon-user=grumio \
        --password=p \
        --wait

    if [ $? -eq 0 ]; then
        failure "The user grumio/plebs should not be able to create a cluster."
    else
        success "  o User 'grumio' is not able to create a cluster, OK."
    fi

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

    end_verbatim
}

function testPingAccess()
{
    local retCode
    print_title "Checking if Outsiders can't Ping Clusters"
    cat <<EOF
  This test will check that an outsider can not ping a cluster it can't even
  see. Then we double check that the owner can actually ping the cluster job.

EOF

    begin_verbatim
    mys9s cluster \
        --ping \
        --cluster-id=1 \
        --log \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to ping a cluster."
    else
        success "  o Outsider can not ping a cluster, ok."
    fi
    
    mys9s cluster \
        --ping \
        --cluster-name="$CLUSTER_NAME" \
        --log \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to ping a cluster."
    else
        success "  o Outsider can not ping the cluster, ok."
    fi
   
    #
    # Checking that the owner on the other hand can create a job.
    #
    mys9s cluster \
        --ping \
        --cluster-id=1 \
        --log 

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        success "  o The owner can ping the cluster, ok."
    else
        failure "The owner could not ping the cluster."
    fi

    end_verbatim
}

function testConfigRead()
{
    print_title "Checking Who has Read Access to Configuration"

    begin_verbatim
    mys9s cluster \
        --list-config \
        --cluster-id=1 \
        'config_file_path'

    if [ $? -eq 0 ]; then
        success "  o The owner has access to the configuration, ok."
    else
        failure "The owner should have access to the configuration."
    fi

    mys9s cluster \
        --list-config \
        --cluster-id=1 \
        --cmon-user=grumio \
        --password=p \
        'config_file_path'

    if [ $? -ne 0 ]; then
        success "  o Outsiders has no access to the configuration, ok."
    else
        failure "Outsiders should have no access to the configuration."
    fi

    mys9s cluster \
        --list-config \
        --cluster-id=0 \
        'config_file_path'

    if [ $? -ne 0 ]; then
        success "  o Normal users has no access to cluster 0, ok."
    else
        failure "Normal users should have no access to cluster 0."
    fi

    mys9s cluster \
        --list-config \
        --cluster-id=0 \
        --cmon-user=system \
        --password=secret \
        'config_file_path'

    if [ $? -eq 0 ]; then
        success "  o System user has access to cluster 0, ok."
    else
        failure "The system user should have no access to cluster 0."
    fi

    end_verbatim
}

function testConfigWrite()
{
    #
    # The write access.
    #
    print_title "Checking Who has Write Access to Cluster Configuration"
    cat <<EOF | paragraph
  New we check who has write access to the cluster configuration. The owner
  should of course be able to change the configuration while the outsider should
  not.
EOF

    mys9s cluster \
        --change-config \
        --cluster-id=1 \
        --cmon-user="grumio" \
        --password="p" \
        --opt-name="host_stats_collection_interval" \
        --opt-value="60"    
    
    if [ $? -ne 0 ]; then
        success "  o The outsider has no write access to cluster 1, ok."
    else
        failure "The outsider should have no write access to cluster 1."
    fi

    mys9s cluster \
        --change-config \
        --cluster-id=1 \
        --opt-name="host_stats_collection_interval" \
        --opt-value="60"    
    
    if [ $? -eq 0 ]; then
        success "  o The owner has write access to cluster 1, ok."
    else
        failure "The owner should have write access to cluster 1."
    fi
    
    mys9s cluster \
        --list-config \
        --cluster-id=1 \
        "host_stats_collection_interval"

    mys9s cluster \
        --change-config \
        --cluster-id=1 \
        --cmon-user="system" \
        --password="secret" \
        --opt-name="host_stats_collection_interval" \
        --opt-value="120"    
    
    if [ $? -eq 0 ]; then
        success "  o The superuser has write access to cluster 1, ok."
    else
        failure "The superuser should have write access to cluster 1."
    fi

    mys9s cluster \
        --list-config \
        --cluster-id=1 \
        "host_stats_collection_interval"

    end_verbatim
}

function testJobAccess()
{
    local retCode

    #
    #
    #
    print_title "Checking if Outsiders can't Create Jobs"
    cat <<EOF
  This test will check that an outsider can not execute a job on the cluster it
  can't even see. Then we double check that the owner can actually execute a
  job.

EOF

    begin_verbatim
    mys9s job \
        --success \
        --cluster-id=1 \
        --log \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to create jobs on cluster."
    else
        success "  o Outsider can not execute job, ok."
    fi
    
    mys9s job \
        --success \
        --cluster-name="$CLUSTER_NAME" \
        --log \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to create jobs on cluster."
    else
        success "  o Outsider can not execute job, ok."
    fi
   
    #
    # Checking that the owner on the other hand can create a job.
    #
    mys9s job \
        --success \
        --cluster-id=1 \
        --log 

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        success "  o The owner can execute a job, ok."
    else
        failure "The owner could not execute a job."
    fi

    end_verbatim
}

function testLogAccess()
{
    local retCode
    print_title "Checking if Outsiders can't See the Logs"

    cat <<EOF
  This test will check that an outsider can not see the logs of a cluster he
  can't see. Then we check that the owner can see the logs.

EOF

    begin_verbatim
    mys9s log \
        --list \
        --cmon-user=grumio \
        --password=p \
        --cluster-id=1 \
        --limit=10 \
        --log-format="%i %M\n"

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not see the logs of the cluster."
    else
        success "  o Outsider can not see the log of the cluster, ok."
    fi

    mys9s log \
        --list \
        --cluster-id=1 \
        --limit=10 \
        --log-format="%i %M\n"
    
    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        success "  o Owner of the cluster can see the logs, ok."
    else
        failure "The owner can't see the logs."
    fi

    end_verbatim
}

#
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

    begin_verbatim
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

                if [ "$mode"  != "brwxrwx---" ]; then
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

                if [ "$mode"  != "brwxrwx---" ]; then
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
                
                if [ "$mode"  != "brwxrwx---" ]; then
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

    end_verbatim
}

function testDatabaseAccess()
{
    local retCode
    
    print_title "Checking that Outsider can not Access Databases"
    cat <<EOF
  This test checks that an outsider can not create new databases and can not
  list the existing databases on a cluster.

EOF

    begin_verbatim
    mys9s cluster \
        --create-database \
        --cluster-name="$CLUSTER_NAME" \
        --db-name="grumio" \
        --cmon-user=grumio \
        --password=p
        
    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to create databases."
    else
        success "  o Outsider can not create database, ok."
    fi

    mys9s cluster \
        --create-database \
        --cluster-id="1" \
        --db-name="grumio" \
        --cmon-user=grumio \
        --password=p
        
    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to create databases."
    else
        success "  o Outsider can not create database, ok."
    fi
    
    mys9s cluster \
        --list-databases \
        --cluster-id="1" \
        --cmon-user=grumio \
        --password=p
        
    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to see databases."
    else
        success "  o Outsider can not see databases, ok."
    fi

    end_verbatim
}

function testBackupAccess()
{
    local retCode

    print_title "Checking that Outsider can not Access Backups"

    begin_verbatim
    mys9s backup \
        --list \
        --long \
        --cluster-id="1" \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not see backups."
    else
        success "  o Outsider can not see backups, ok."
    fi
    
    mys9s backup \
        --list \
        --long \
        --cluster-name="$CLUSTER_NAME" \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not see backups."
    else
        success "  o Outsider can not see backups, ok."
    fi
 
    #
    # Creating a backup that we can try to delete.
    #
    mys9s backup \
        --create \
        --cluster-id="1" \
        --backup-dir=/tmp \
        --nodes="$CONTAINER_IP" \
        --wait
    
    check_exit_code $?

    mys9s backup --list --long 

    #
    #
    #
    mys9s backup \
        --delete \
        --backup-id=1 \
        --cmon-user=grumio \
        --password=p
    
    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not delete backups."
    else
        success "  o Outsider can not delete backups, ok."
    fi

    end_verbatim
}

#
# Creating a database account.
#
function testCreateAccount()
{
    print_title "Creating Database Account"
    cat <<EOF
  This test will create an account on the cluster and checks the return value to
  see if the operation was indeed successfull.

EOF

    begin_verbatim
    mys9s account \
        --create \
        --cluster-name="galera_001" \
        --account="pipas:pipas" \
        --privileges="*.*:ALL"

    check_exit_code_no_job $?
    end_verbatim
}

function testConfigController()
{
    local controller_name

    print_title "Testing the Configuration of the Controller"

    begin_verbatim
    controller_name=$(\
        s9s node --list --node-format "%R %A\n" | \
        grep controller | \
        awk '{print $2}')
    end_verbatim
}

#
# FIXME: There is an other, a better test for this in ft_cluster_lxc.sh.
#
function testAlarmAccess()
{
    local retCode

    print_title "Checking if Outsiders can See the Alarms"

    begin_verbatim

    #
    # Checking the creating of accounts.
    #
    mys9s alarm \
        --list \
        --long \
        --cluster-id=1 \
        --cmon-user="grumio" 

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not see the alarms."
    else
        success "  o Outsider can not see the alarms, ok."
    fi

    end_verbatim
}

function testReportAccess()
{
    print_title "Checking Report Creation and Privileges"

    begin_verbatim

    #
    # Checking the creating of accounts.
    #
    mys9s report --create --type=testreport --cluster-id=1
    mys9s report --create --type=report1    --cluster-id=1
    if [ "$?" -eq 0 ]; then
        success "  o The owner can create a report, ok."
    else
        failure "The owner should be able to create a report."
    fi
    
    mys9s report --create --type=testreport --cluster-id=1 --cmon-user=grumio
    if [ "$?" -eq 0 ]; then
        warning "Outsiders should not be able to create reports."
    else
        success "  o Outsider can not create a report, ok."
    fi

    #
    # Listing reports by cluster ID.
    #
    mys9s report --list --long --cluster-id=1
    if [ "$?" -eq 0 ]; then
        success "  o The owner can see the reports, ok."
    else
        failure "The owner should be able to see the reports."
    fi

    mys9s report --list --long --cluster-id=1 --cmon-user=grumio
    if [ "$?" -eq 0 ]; then
        warning "Outsiders should not be able to see reports."
    else
        success "  o Outsider can not see the reports, ok."
    fi

    #
    # Viewing the report text.
    #
    mys9s report --cat --report-id=1
    if [ "$?" -eq 0 ]; then
        success "  o The owner can see the reports, ok."
    else
        failure "The owner should be able to see the reports."
    fi

    mys9s report --cat --report-id=1 --cmon-user=grumio
    if [ "$?" -eq 0 ]; then
        warning "Outsiders should not be able to see reports."
    else
        success "  o Outsider can not see the reports, ok."
    fi

    #
    # Deleting reports.
    #
    mys9s report --delete --report-id=1 --cmon-user=grumio
    if [ "$?" -eq 0 ]; then
        warning "Outsiders should not be able to delete reports."
    else
        success "  o Outsider can not delete report, ok."
    fi

    mys9s report --delete --report-id=2 
    if [ "$?" -eq 0 ]; then
        success "  o The owner can delete a report, ok."
    else
        failure "The owner should be able to delete a report."
    fi

    end_verbatim
}



function testAccountAccess()
{
    local retCode

    print_title "Checking if Outsiders can Access Accounts"

    begin_verbatim

    #
    # Checking the creating of accounts.
    #
    mys9s account \
        --create \
        --cluster-id=1 \
        --account="grumio:grumio" \
        --privileges="*.*:ALL" \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to create accounts."
    else
        success "  o Outsider can not create account, ok."
    fi
    
    mys9s account \
        --create \
        --cluster-name="galera_001" \
        --account="grumio:grumio" \
        --privileges="*.*:ALL" \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not be able to create accounts."
    else
        success "  o Outsider can not create account, ok."
    fi

    #
    #  Checking the read access to the accounts.
    #
    mys9s account \
        --list \
        --long \
        --cluster-id=1 \
        --cmon-user=grumio \
        --password=p

    retCode=$?
    if [ "$retCode" -eq 0 ]; then
        warning "Outsiders should not see the accounts."
    else
        success "  o Outsider can not see the accounts, ok."
    fi

    end_verbatim
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
    cat <<EOF | paragraph
In this scenario the user moves some objects into a subfolder, then moves the
user object itself. All the objects should remain visible in a chroot
environment.
EOF

    begin_verbatim
    
    mys9s tree --list --long --recursive --full-path --batch

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
        line=$(echo "$line" | sed 's/1, 0/   -/g')
        line=$(echo "$line" | sed 's/3, 1/   -/g')
        name=$(echo "$line" | awk '{print $5}')
        mode=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $3}')
        group=$(echo "$line" | awk '{print $4}')

        success "  o Line is: $line"
        case "$name" in 
            /$CLUSTER_NAME)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "crwxrwx---" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /$CLUSTER_NAME/databases/domain_names_diff)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrwx---" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /$CONTAINER_SERVER)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "srwxrwx---" ] && failure "Mode is '$mode'." 
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
                [ "$mode"  != "crwxrwx---" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /home/pipas/$CONTAINER_SERVER)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "srwxrwx---" ] && failure "Mode is '$mode'." 
                let n_object_found+=1
                ;;

            /home/pipas/$CLUSTER_NAME/databases/domain_names_diff)
                [ "$owner" != "pipas" ] && failure "Owner is '$owner'."
                [ "$group" != "users" ] && failure "Group is '$group'."
                [ "$mode"  != "brwxrwx---" ] && failure "Mode is '$mode'." 
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

    end_verbatim
}

function testStats()
{
    print_title "Checking CDT Path of Objects"

    begin_verbatim
    mys9s user --stat "$USER"
    
    mys9s server --stat
    mys9s server --list --long
    
    mys9s cluster --stat

    mys9s server --list --long
    end_verbatim
}

#
# Checking the --get-acl in a chroot environment.
#
function testAclChroot()
{
    print_title "Checking getAcl replies"

    begin_verbatim
    s9s tree --get-acl --print-json /

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

    end_verbatim
}

#####
# Creating a directory.
#
function testCreateFolder()
{
    print_title "Creating directory"

    begin_verbatim
    mys9s tree \
        --mkdir \
        /tmp

    check_exit_code_no_job $?
    end_verbatim
}

#####
# Adding an ACL.
#
function testAddAcl()
{
    print_title "Adding an ACL Entry"

    begin_verbatim
    mys9s tree --add-acl --acl="user:pipas:rwx" /tmp
    check_exit_code_no_job $?

    mys9s tree --get-acl /tmp
    end_verbatim
}

function testChangeOwner()
{
    #
    # Changing the owner
    #
    print_title "Changing the Owner"

    begin_verbatim
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

    end_verbatim
}

#####
# A final view and exit.
#
function testTree()
{
    local name
    local n_object_found=0

    print_title "Printing and Checking Tree"
    
    begin_verbatim
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

    end_verbatim
}

function testMoveBack()
{
    print_title "Moving Objects from Chroot"
    cat <<EOF
Here the superuser moves the objects back into the root directory out from the
chroot environment of the other user. The other user should not see the 
objects.
EOF

    begin_verbatim
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

    end_verbatim
}

#
# Running the requested tests.
#
startTests
create_s9s_config

if [ "$OPTION_INSTALL" ]; then
    if [ "$*" ]; then
        for testName in $*; do
            runFunctionalTest "$testName"
        done
    else
        runFunctionalTest testCreateUser
        runFunctionalTest testCreateSuperuser
        runFunctionalTest testCreateOutsider
        runFunctionalTest testLicenseDevice
        runFunctionalTest testRegisterServer
        runFunctionalTest testCreateContainer
        runFunctionalTest testCreateCluster
        runFunctionalTest testPingAccess
        runFunctionalTest testConfigRead
        runFunctionalTest testConfigWrite
        runFunctionalTest testJobAccess
        runFunctionalTest testLogAccess
        runFunctionalTest testAccountAccess
        runFunctionalTest testDatabaseAccess
        runFunctionalTest testBackupAccess
        runFunctionalTest testAlarmAccess
        runFunctionalTest testReportAccess
    fi
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateUser
    runFunctionalTest testCreateSuperuser
    runFunctionalTest testCreateOutsider
    runFunctionalTest testLicense
    runFunctionalTest testLicenseUpload

    runFunctionalTest testRegisterServer
    runFunctionalTest testCreateContainer
    runFunctionalTest testCreateCluster
    runFunctionalTest testPingAccess
    runFunctionalTest testConfigRead
    runFunctionalTest testConfigWrite
    runFunctionalTest testJobAccess
    runFunctionalTest testLogAccess
    runFunctionalTest testCreateDatabase
    runFunctionalTest testDatabaseAccess
    runFunctionalTest testBackupAccess
    runFunctionalTest testAlarmAccess
    runFunctionalTest testReportAccess
    runFunctionalTest testCreateAccount
    runFunctionalTest testAccountAccess
    runFunctionalTest testMoveObjects
    runFunctionalTest testStats
    runFunctionalTest testAclChroot
    runFunctionalTest testCreateFolder
    runFunctionalTest testAddAcl
    runFunctionalTest testChangeOwner
    runFunctionalTest testTree
    runFunctionalTest testMoveBack
    runFunctionalTest deleteContainers
fi

endTests

