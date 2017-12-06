#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CONTAINER_SERVER=""

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

if [ -z "$S9S" ]; then
    printError "The s9s program is not installed."
    exit 7
fi

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

#####
# Creating a user to be a normal user. 
#
print_title "Creating a user with normal privileges."
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

check_exit_code $?

#####
# Creating a user to be a new superuser.
#
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
    "admin"

check_exit_code $?

#####
# Moving the normal user in the tree.
#
print_title "The user moves itself in CDT"

TEST_PATH="/home/pipas"
mys9s tree --mkdir "$TEST_PATH"
mys9s tree --move /pipas "$TEST_PATH"

check_exit_code $?

#####
# Registering second server.
#
print_title "Registering a container server."
mys9s server \
    --register \
    --cmon-user=pipas \
    --servers="lxc://$CONTAINER_SERVER"

check_exit_code $?

OWNER=$(s9s tree --list /$CONTAINER_SERVER --batch | head -n1 | awk '{print $2}')
GROUP=$(s9s tree --list /$CONTAINER_SERVER --batch | head -n1 | awk '{print $3}')
if [ "$OWNER" != 'pipas' ]; then
    s9s tree --list /$CONTAINER_SERVER 
    failure "The owner should be 'pipas'."
    exit 1
fi

if [ "$GROUP" != 'users' ]; then
    s9s tree --list /$CONTAINER_SERVER 
    failure "The group should be 'users'."
    exit 1
fi

#####
# Creating a container.
#
print_title "Creating a Container."
mys9s container \
    --create \
    --wait container_002

check_exit_code $?

CONTAINER_IP=$(\
    s9s server \
        --list-containers \
        --long container_002 \
        --batch \
    | awk '{print $7}')

if [ -z "$CONTAINER_IP" ]; then
    failure "Container IP could not be found."
    exit 1
fi

if [ "$CONTAINER_IP" == "-" ]; then
    failure "Container IP is invalid."
    # Not an IP, but it should also work in destroyNodes()
    ALL_CREATED_IPS+=" container_002"
    exit 1
else
    ALL_CREATED_IPS+="$CONTAINER_IP"
fi

#####
# Creating a Galera cluster.
#
print_title "Creating an other cluster."
mys9s cluster \
    --create \
    --cluster-type=galera \
    --nodes="$CONTAINER_IP" \
    --vendor=percona \
    --cluster-name="galera_002" \
    --provider-version=5.6 \
    --wait

check_exit_code $?

OWNER=$(s9s tree --list /galera_002 --batch | head -n1 | awk '{print $2}')
GROUP=$(s9s tree --list /galera_002 --batch | head -n1 | awk '{print $3}')
if [ "$OWNER" != 'pipas' ]; then
    mys9s tree --list /galera_002
    failure "The owner of '/galera_002' should be 'pipas'."
    exit 1
fi

if [ "$GROUP" != 'users' ]; then
    mys9s tree --list /galera_002
    failure "The group of '/galera_002' should be 'users'."
    exit 1
fi

mys9s tree --tree
mys9s tree --list 

endTests
