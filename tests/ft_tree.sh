#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
S9S_CONFIG_DIR="$HOME/.s9s"

cd $MYDIR
source include.sh

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
 --reset-config   Remove and re-generate the ~/.s9s directory.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config" \
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
    printError "This test needs the --reset-config to run."
    exit 6
fi

reset_config

#
# Creating a user to be a normal user. 
#
echo "Creating a user with normal privileges."
mys9s user \
    --create \
    --cmon-user="system" \
    --password="secret" \
    --group="users" \
    --create-group \
    --email-address="laszlo@severalnines.com" \
    --first-name="László" \
    --last-name="Pere"   \
    --generate-key \
    --new-password="pipas" \
    "pipas"

#
# Creating a user to be a new superuser.
#
echo "Creating a user with superuser privileges."
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

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while creating user through RPC"
    exit 1
fi

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while creating user through RPC"
    exit 1
fi

#
# Registering a server.
#
echo "Registering a container server."
mys9s server \
    --register \
    --servers="lxc://core1"

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while registering server."
    exit 1
fi

#
# Moving the server.
#
#echo "Moving the server in the tree"
#mys9s server \
#    --move \
#    /core1 /servers
#
#exitCode=$?
#if [ "$exitCode" -ne 0 ]; then
#    failure "The exit code is ${exitCode} while moving server."
#    exit 1
#fi

#
# Creating a container.
#
echo "Creating a container."
mys9s server \
    --create container_001

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while creating container."
    exit 1
fi

CONTAINER_IP=$(\
    s9s server \
        --list-containers \
        --long container_001 \
        --batch \
    | awk '{print $7}')

echo "CONTAINER_IP : $CONTAINER_IP"

#
# Creating a Galera cluster.
#
echo "Creating a cluster."
mys9s cluster \
    --create \
    --cluster-type=galera \
    --nodes="$CONTAINER_IP" \
    --vendor=percona \
    --cluster-name="galera_001" \
    --provider-version=5.6 \
    --wait

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while creating container."
    exit 1
fi

#
# Create databases.
#
mys9s cluster \
    --create-database \
    --cluster-name="galera_001" \
    --db-name="domain_names_ngtlds_diff" \
    --batch

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while creating database."
    exit 1
fi

#
# Creating a database account.
#
mys9s account \
    --create \
    --cluster-name="galera_001" \
    --account="pipas:pipas" \
    --privileges="*.*:ALL"

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode} while creating account."
    exit 1
fi

mys9s tree --tree --refresh

