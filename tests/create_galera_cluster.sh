#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CONTAINER_SERVER=""

cd $MYDIR
source ./include.sh

PROVIDER_VERSION=$PERCONA_GALERA_DEFAULT_PROVIDER_VERSION

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Test script for s9s to check Galera clusters.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --log            Print the logs while waiting for the job to be ended.
 --server=SERVER  The name of the server that will hold the containers.
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,log,server:,print-commands,reset-config" \
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
            ;;

        --log)
            shift
            LOG_OPTION="--log"
            ;;

        --server)
            shift
            CONTAINER_SERVER="$1"
            shift
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

if [ "$*" ]; then
    printError "Extra command line argument."
    exit 6
fi

if [ -z "$S9S" ]; then
    echo "The s9s program is not installed."
    exit 7
fi

if [ -z $(which pip-container-create) ]; then
    printError "The 'pip-container-create' program is not found."
    printError "Don't know how to create nodes, giving up."
    exit 1
fi

if [ "$OPTION_RESET_CONFIG" ]; then
    reset_config
    grant_user
fi

cluster_name="cluster_$$"

#
# Creating a Galera cluster.
# FIXME: It is possible to create two clusters with the same name.
#
pip-say "Creating containers."

nodes=""
nodes+="$(create_node "galera_node_01")"
nodes+=";$(create_node "galera_node_02")"
nodes+=";$(create_node "galera_node_03")"

pip-say "Installing cluster."
mys9s cluster \
    --create \
    --cluster-type=galera \
    --nodes="$nodes" \
    --vendor=percona \
    --cluster-name="$cluster_name" \
    --provider-version=$PROVIDER_VERSION \
    $LOG_OPTION


#
# 
#
pip-say "Creating database and account."
dbname="mydatabase"

mys9s cluster \
    --create-database \
    --cluster-name=$cluster_name \
    --db-name=$dbname 

#
#
#
account=$USER
password="password"

mys9s account \
    --create \
    --cluster-name=$cluster_name \
    --account="$account:$password" \
    --privileges="$dbname.*:ALL" \
    --batch

#
# FIXME: This is not working: it says cluster 0 is not running!
#    --cluster-name=$cluster_name \
#
#pip-say "Installing proxy server."
#
#haproxy="$(create_node "haproxy_node_01")"
#
#mys9s cluster \
#    --add-node \
#    --cluster-id=1 \
#    --nodes="haProxy://$haproxy" \
#    $LOG_OPTION
#

pip-say "Finished installing cluster."
echo "cluster_name : $cluster_name"
echo "    db_nodes : $nodes"
#echo "     haproxy : $haproxy"
echo "      dbname : $dbname"
echo "     account : $account"
echo "    password : $password"
