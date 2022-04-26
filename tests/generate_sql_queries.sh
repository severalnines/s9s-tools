#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""


OPTION_PROXYSQL=""
IS_MYSQL=""
IS_POSTGRESQL=""
CLUSTER_ID=""
CLUSTER_TYPE=""
SQL_CLASS=""
SQL_HOST=""
SQL_PORT=""

DB_ALREADY_CREATED=""
db_name="${PROJECT_OWNER}1_$$"
user_name="${PROJECT_OWNER}1_$$"
password="p"

cd $MYDIR
source ./include.sh

unset S9S_DEBUG_PRINT_REQUEST
DONT_PRINT_TEST_MESSAGES="true"
PRINT_COMMANDS="true"


#run queres, 10 clients...  10000 seconds.. -j no idea.
#pgbench -c 10 -j 2 -t 10000 





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
  --proxysql       Generate traffix through the ProxySql server.

SUPPORTED TEST NAMES
  collectData
  testCreateDatabase
  testCreateTable
  testInsert

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,proxysql" \
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

        --proxysql)
            OPTION_PROXYSQL="true"
            shift
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

    if [ -n "$DB_ALREADY_CREATED" ]; then
        cat <<EOF
  The ProxySql is tested with pre-created database and account, so we are not
  going to create them here.
EOF
        return 0
    fi

    mys9s node --list --long

    #
    # Creating a new database on the cluster.
    #
    if [ -z "$SQL_HOST" ]; then
        failure "No SQL host is set."
    else
        mys9s cluster \
            --create-database \
            --cluster-id="$CLUSTER_ID" \
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
            --cluster-id="$CLUSTER_ID" \
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
    elif [ -n "$IS_MYSQL" ]; then
        success "  o Using MySQL client, OK."
        query="CREATE TABLE IF NOT EXISTS $table_name(NAME CHAR(255) NOT NULL, VALUE INT);"

        reply=$(\
            mysql -h$SQL_HOST -P$SQL_PORT -u$user_name -p$password $db_name -e "$query")

        if [ $? -ne 0 ]; then
            failure "Failed to create table"
            failure "Command: mysql -h$SQL_HOST -P$SQL_PORT -u$user_name -p$password $db_name -e \"$query\""
        else
            success "  o Created table, ok."
        fi
    elif [ -n "$IS_POSTGRESQL" ]; then
        success "  o PostgreSQL client will be used, OK."
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

    for file in *; do
        if [ "$file" == "-" ]; then
            continue
        fi

        query="INSERT INTO $table_name(NAME, VALUE) VALUES('$file', 10);"

        reply=$(\
            mysql \
                -h$SQL_HOST \
                -P$SQL_PORT \
                -u$user_name \
                -p$password \
                $db_name \
                -e "$query")

        if [ $? -ne 0 ]; then
            failure "Failed to insert."
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

    #
    #
    #
        query="SELECT 42;"
        reply=$(\
            mysql \
                -h$SQL_HOST \
                -P$SQL_PORT \
                -u$user_name \
                -p$password \
                $db_name \
                -e "$query")
        
        query="SELECT 41 + 1;"
        reply=$(\
            mysql \
                -h$SQL_HOST \
                -P$SQL_PORT \
                -u$user_name \
                -p$password \
                $db_name \
                -e "$query")

        query="SELECT NAME, VALUE FROM $table_name WHERE NAME = 'ak';"
        reply=$(\
            mysql \
                -h$SQL_HOST \
                -P$SQL_PORT \
                -u$user_name \
                -p$password \
                $db_name \
                -e "$query")

}

n=0
function testInsertPostgreSql()
{
    local table_name="test_table"
    local query

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
    elif [ -n "$IS_MYSQL" ]; then
        testInsertMySql 
    elif [  -n "$IS_POSTGRESQL" ]; then
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
    print_title "Finding Information"
  
    #
    # The cluster ID
    #
    CLUSTER_ID=$(\
        s9s cluster --list --long --batch --cluster-format="%I\n" |\
        head -n 1)
    if [ -n "$CLUSTER_ID" ]; then
        success "  o Cluster ID is $CLUSTER_ID, OK."
    else
        failure "Cluster ID was not found."
    fi
    
    CLUSTER_TYPE=$(\
        s9s cluster --list --long --batch --cluster-id=$CLUSTER_ID --cluster-format="%T\n")
    if [ -n "$CLUSTER_TYPE" ]; then
        success "  o Cluster type is $CLUSTER_TYPE, OK."
    else
        failure "Cluster type was not found."
    fi

    #
    # The SQL HOST.
    #
    if [ -n "$OPTION_PROXYSQL" ]; then
        SQL_CLASS="CmonProxySqlHost"
        SQL_HOST=$(\
            s9s node --list --properties="class_name=$SQL_CLASS" | \
            head -n1)
        #
        # The following tests create this account and db in advance:
        #   o ft_proxysql_connect.sh
        #
        #user_name="${PROJECT_OWNER}"
        #password="password"
        #db_name="test_database"
        #DB_ALREADY_CREATED="true"

        # 
        SQL_PORT="6033"
    else
        if [ -z "$SQL_HOST" ]; then
            SQL_CLASS="CmonGaleraHost"
            SQL_HOST=$(\
                s9s node --list --properties="class_name=$SQL_CLASS" | \
                head -n1)
        fi
        
        if [ -z "$SQL_HOST" ]; then
            SQL_CLASS="CmonMySqlHost"
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
    fi

    if [ -z "$SQL_HOST" ]; then
        failure "Could not find SQL host."
        SQL_CLASS=""
        return 1
    else
        success "  o SQL host is '$SQL_HOST', ok."
    fi

    if [ -z "$SQL_CLASS" ]; then
        failure "No SQL server class set."
    else
        success "  o SQL node class is $SQL_CLASS, OK."
    fi

    #
    # The SQL port.
    #
    if [ -z "$SQL_PORT" ]; then
        SQL_PORT=$(s9s node --list --node-format="%P\n" "$SQL_HOST")
    fi

    if [ -z "$SQL_PORT" ]; then
        failure "Could not find SQL port."
    else
        success "  o SQL port is $SQL_PORT, ok."
    fi
    
    if [ "$SQL_CLASS" == "CmonGaleraHost" ]; then
        IS_MYSQL="true"
    elif [ "$SQL_CLASS" == "CmonMySqlHost" ]; then
        IS_MYSQL="true"
    elif [ "$SQL_CLASS" == "CmonPostgreSqlHost" ]; then
        IS_POSTGRESQL="true"
    elif [ "$SQL_CLASS" == "CmonProxySqlHost" ]; then
        IS_MYSQL="true"
    fi

    if [ -n "$IS_MYSQL" ]; then
        success "  o MySQL communication will be used, OK."
    elif [ -n "$IS_POSTGRESQL" ]; then
        success "  o PostgreSQL communication will be used, OK."
    else
        failure "Could not determine what SQL client to use."
    fi
}

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest collectData
    runFunctionalTest testCreateDatabase
    runFunctionalTest testCreateTable

    pushd "$HOME"
    runFunctionalTest testInsert
    popd
fi


