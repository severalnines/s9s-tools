#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
SQL_HOST=""

cd $MYDIR
source ./include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Tests the cluster under load.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose" \
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

        --)
            shift
            break
            ;;
    esac
done

#
# This test will create a user and a database and then upload some data if the
# data can be found on the local computer.
#
function testUploadData()
{
    local sql_host=$1
    local db_name="${S9STEST_USER}1_$$"
    local user_name="${S9STEST_USER}1_$$"
    local password="p"
    local reply
    local count=0

    print-title "Importing Data"

    #
    # Creating a new database on the cluster.
    #
    mys9s cluster \
        --create-database \
        --db-name=$db_name
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
        return 1
    fi

    #
    # Creating a new account on the cluster.
    #
    mys9s account \
        --create \
        --account="$user_name:$password" \
        --privileges="$db_name.*:ALL"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating account."
    fi

    #
    # Executing a simple SQL statement using the account we created.
    #
    reply=$(\
        mysql \
            --disable-auto-rehash \
            --batch \
            -h$sql_host \
            -u$user_name \
            -p$password \
            $db_name \
            -e "SELECT 41+1" | tail -n +2 )

    if [ "$reply" != "42" ]; then
        echo "Cluster failed to execute an SQL statement: '$reply'."
    fi

    #
    # Here we upload some tables. This part needs test data...
    #
    for file in \
        /home/${S9STEST_USER}/Desktop/stuff/databases/*.sql.gz \
        /home/domain_names_ngtlds_diff_whois/*/*/*.sql.gz \
        ; do
        if [ ! -f "$file" ]; then
            continue
        fi

        printf "%'6d " "$count"
        printf "$XTERM_COLOR_RED$file$TERM_NORMAL"
        printf "\n"
        zcat $file | mysql --batch -h$sql_host -u$user_name -pp $db_name

        exitCode=$?
        if [ "$exitCode" -ne 0 ]; then
            failure "Exit code is $exitCode while uploading data."
            break
        fi

        let count+=1
        if [ "$count" -gt 999 ]; then
            break
        fi
    done
}

if [ -z "$SQL_HOST" ]; then
    SQL_HOST=$(\
        s9s node --list --properties="class_name=CmonGaleraHost" | \
        head -n1)
fi

if [ -z "$SQL_HOST" ]; then
    printError "Could not find SQL host."
    exit 5
fi

if [ -z "$SQL_PORT" ]; then
    SQL_PORT=$(s9s node --list --node-format="%P\n" "$SQL_HOST")
fi

if [ -z "$SQL_PORT" ]; then
    printError "Could not find SQL port."
    exit 5
fi

printVerbose " SQL_HOST : '$SQL_HOST'"
printVerbose " SQL_PORT : $SQL_PORT"
testUploadData "$SQL_HOST"
