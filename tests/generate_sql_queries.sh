#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""

SQL_CLASS=""
SQL_HOST=""
SQL_PORT=""

db_name="pipas1_$$"
user_name="pipas1_$$"
password="p"

cd $MYDIR
source ./include.sh

unset S9S_DEBUG_PRINT_REQUEST

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
function testCreateDatabase()
{
    local password="p"
    local reply
    local count=0

    print_title "Creating Database & Account"

    #
    # Creating a new database on the cluster.
    #
    if [ -z "$SQL_HOST" ]; then
        failure "No SQL host is set."
    else
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
    fi
}

function testCreateTable()
{
    local table_name="test_table"
    local query

    print_title "Creating SQL Table"

    if [ -z "$SQL_HOST" ]; then
        failure "No SQL host is set."
    elif [ "$SQL_CLASS" == "CmonGaleraHost" ]; then
        success "  o SQL class is $SQL_CLASS, ok."
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
    elif [ "$SQL_CLASS" == "CmonPostgreSqlHost" ]; then
        success "  o SQL class is $SQL_CLASS, ok."
        query="CREATE TABLE $table_name(NAME CHAR(255) NOT NULL, VALUE INT);"

        reply=$( \
            PGPASSWORD="$password" \
            psql \
                -t \
                --username="$user_name" \
                --host="$SQL_HOST" \
                --port="$SQL_PORT" \
                --dbname="$db_name" \
                --command="$query")

        if [ $? -ne 0 ]; then
            failure "Failed to create table"
        else
            success "  o Created table, ok."
        fi
    else
        failure "SQL host class $SQL_CLASS is not handled here."
    fi
}

function testInsertMySql()
{
    local table_name="test_table"
    local query
    local file
    #print_title "Creating SQL Table"

    for file in *; do
        if [ "$file" == "-" ]; then
            continue
        fi

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
            #exit 5
        else
            success "  o success: $query."
        fi

        if [ -d "$file" ]; then
            echo "Entering directory $file..."
            pushd "$file" >/dev/null 2>/dev/null
            if [ $? -eq 0 ]; then
                testInsertMySql
                popd >/dev/null 2>/dev/null
            else
                echo "Could not enter directory $file..."
            fi
        fi
    done
}

n=0
function testInsertPostgreSql()
{
    local table_name="test_table"
    local query
    #print_title "Creating SQL Table"

    for file in *; do
        if [ "$file" == "-" ]; then
            continue
        fi

        query="INSERT INTO $table_name(NAME, VALUE) VALUES('$file', 10);"

        reply=$( \
            PGPASSWORD="$password" \
            psql \
                -t \
                --username="$user_name" \
                --host="$SQL_HOST" \
                --port="$SQL_PORT" \
                --dbname="$db_name" \
                --command="$query")

        if [ $? -ne 0 ]; then
            failure "Failed to insert."
            #exit 5
        else
            success "  o $n: success: $query."
        fi

        if [ "$n" -gt 10 ]; then
            #
            #
            #
            query="SELECT COUNT(*) FROM $table_name WHERE NAME ~* '.*gz' OR NAME ~* '.*pdf';"

            reply=$( \
                PGPASSWORD="$password" \
                psql \
                    -t \
                    --username="$user_name" \
                    --host="$SQL_HOST" \
                    --port="$SQL_PORT" \
                    --dbname="$db_name" \
                    --command="$query")
        
            if [ $? -ne 0 ]; then
                failure "Failed to select."
                #exit 5
            else
                success "  o $reply success: $query."
            fi

            let n=0
        else
            let n+=1
        fi

        if [ -d "$file" ]; then
            #
            #
            #
            echo "Entering directory $file..."            
            pushd $file >/dev/null 2>/dev/null
            if [ $? -eq 0 ]; then
                testInsertPostgreSql
                popd >/dev/null 2>/dev/null
            fi

        fi
    done
}

function testInsert()
{
    if [ -z "$SQL_HOST" ]; then
        failure "No SQL host is set."
    elif [ -z "$SQL_CLASS" ]; then
        failure "The class for SQL host is not set."
    elif [ "$SQL_CLASS" == "CmonGaleraHost" ]; then
        testInsertMySql 
    elif [ "$SQL_CLASS" == "CmonPostgreSqlHost" ]; then
        testInsertPostgreSql
    else
        failure "SQL host class $SQL_CLASS is not handled here."
    fi
}

#
# Looks around and finds the cluster and the node on which the SQL commands will
# be executed.
#
function collectData()
{
    print_title "Finding the SQL Server"

    if [ -z "$SQL_HOST" ]; then
        SQL_CLASS="CmonGaleraHost"
        SQL_HOST=$(\
            s9s node --list --properties="class_name=$SQL_CLASS" | \
            head -n1)
    fi

    if [ -z "$SQL_HOST" ]; then
        SQL_CLASS="CmonPostgreSqlHost"
        SQL_HOST=$(\
            s9s node --list --properties="class_name=$SQL_CLASS" | \
            head -n1)
    fi


    if [ -z "$SQL_HOST" ]; then
        failure "Could not find SQL host."
        SQL_CLASS=""
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

pushd "$HOME"
runFunctionalTest testInsert
popd

