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

SQL_CLASS=""
SQL_HOST=""
SQL_PORT=""

db_name="pipas1_$$"
user_name="pipas1_$$"
password="p"

#
# This test will create a user and a database and then upload some data if the
# data can be found on the local computer.
#
function testCreateDatabase()
{
    local password="p"
    local reply
    local count=0

    print_title "Creating Database & Account"

    #
    # Creating a new database on the cluster.
    #

    mys9s cluster \
        --create-database \
        --batch \
        --db-name=$db_name
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating a database."
    else
        success "  o Created database $db_name, ok."
    fi
    
    #
    # Creating a new account on the cluster.
    #
    mys9s account \
        --create \
        --batch \
        --account="$user_name:$password" \
        --privileges="$db_name.*:ALL"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is $exitCode while creating account."
    else
        success "  o Created account $user_name, ok."
    fi
}

function testCreateTable()
{
    local table_name="test_table"
    local query

    print_title "Creating SQL Table"

    query="CREATE TABLE $table_name(NAME CHAR(255) NOT NULL, VALUE INT);"

    reply=$(\
        mysql \
            --disable-auto-rehash \
            --batch \
            -h$SQL_HOST \
            -u$user_name \
            -p$password \
            $db_name \
            -e "$query")

    if [ $? -ne 0 ]; then
        failure "Failed to create table"
    else
        success "  o Created table, ok."
    fi
}

function testInsert()
{
    local table_name="test_table"
    local query

    #print_title "Creating SQL Table"

    for file in *; do
        query="INSERT INTO $table_name(NAME, VALUE) VALUES('$file', 10);"

        reply=$(\
            mysql \
                --disable-auto-rehash \
                --batch \
                -h$SQL_HOST \
                -u$user_name \
                -p$password \
                $db_name \
                -e "$query")

        if [ $? -ne 0 ]; then
            failure "Failed to insert."
            exit 5
        else
            success "  o success: $query."
        fi

        if [ -d "$file" ]; then
            pushd $file >/dev/null 2>/dev/null
            if [ $? -eq 0 ]; then
                testInsert
                popd >/dev/null 2>/dev/null
            fi
        fi
    done
}

unset S9S_DEBUG_PRINT_REQUEST

function collectData()
{
    print_title "Collecting Data"

    if [ -z "$SQL_HOST" ]; then
        SQL_CLASS="CmonGaleraHost"
        SQL_HOST=$(\
            s9s node --list --properties="class_name=$SQL_HOST" | \
            head -n1)
    fi

    if [ -z "$SQL_HOST" ]; then
        failure "Could not find SQL host."
        return 1
    else
        success "  o Sql host is '$SQL_HOST', ok."
    fi

    if [ -z "$SQL_PORT" ]; then
        SQL_PORT=$(s9s node --list --node-format="%P\n" "$SQL_HOST")
    fi

    if [ -z "$SQL_PORT" ]; then
        failure "Could not find SQL port."
    else
        success "  o SQL port is $SQL_PORT, ok."
    fi
}

runFunctionalTest collectData
runFunctionalTest testCreateDatabase
runFunctionalTest testCreateTable

pushd "/home"
runFunctionalTest testInsert
popd

