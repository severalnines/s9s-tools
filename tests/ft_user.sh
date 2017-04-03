#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")

# This is the name of the server that will hold the linux containers.
CONTAINER_SERVER="core1"

FIRST_ADDED_NODE=""
LAST_ADDED_NODE=""

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

 -h, --help      Print this help and exit.
 --verbose       Print more messages.
 --print-json    Print the JSON messages sent and received.
 --log           Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands" \
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

        --)
            shift
            break
            ;;
    esac
done

if [ -z "$S9S" ]; then
    echo "The s9s program is not installed."
    exit 7
fi

CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

#
# 
#
function index_table()
{
    local text="$1"
    local row=$2
    local column=$3
    local line
    local counter=1

    line=$(echo "$text" | head -n $row | tail -n 1)
    for cell in $line; do
        if [ $counter -eq "$column" ]; then
            echo "$cell"
            break
        fi

        let counter+=1
    done
}

#
#
#
function testPing()
{
    pip-say "Pinging controller."

    #
    # Pinging. 
    #
    mys9s cluster \
        --ping \
        $OPTION_PRINT_JSON \
        $OPTION_VERBOSE

    exitCode=$?
    printVerbose "exitCode = $exitCode"
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
        pip-say "The controller is off line. Further testing is not possible."
    else
        pip-say "The controller is on line."
    fi
}

#
# Just a normal createUser call we do all the time to register a user on the
# controller so that we can actually execute RPC calls.
#
function testGrantUser()
{
    mys9s user \
        --create \
        --cmon-user=$USER \
        --generate-key \
        $OPTION_PRINT_JSON \
        $OPTION_VERBOSE \
        --batch

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while granting user."
        return 1
    fi
}

#
# This test will check the system users, users that should be available on every
# system.
#
function testSystemUsers()
{
    local text
    local row
    local column
    local required

    # This command 
    #$S9S user --list --long 
    text=$($S9S user --list --long --batch --color=never)

    row=1
    column=2
    cell=$(index_table "$text" $row $column)
    required="1"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        return 1
    fi

    row=1
    column=3
    cell=$(index_table "$text" $row $column)
    required="system"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        return 1
    fi
    
    row=1
    column=4
    cell=$(index_table "$text" $row $column)
    required="admins"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        return 1
    fi
    
    row=2
    column=2
    cell=$(index_table "$text" $row $column)
    required="2"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        return 1
    fi
    
    row=2
    column=3
    cell=$(index_table "$text" $row $column)
    required="nobody"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        return 1
    fi
    
    row=2
    column=4
    cell=$(index_table "$text" $row $column)
    required="nobody"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        return 1
    fi
}

#
# Testing what happens when a creation of a new user fails because the group 
# does not exist.
#
function testFailNoGroup()
{
    local user_name

    #
    # Yes, this is a problem, we can't get an error message back from the pipe.
    # The group here does not exists and we did not request the creation of the
    # group, so this will fail, but we still get the AOK back from the program.
    #
    mys9s user \
        --create \
        --cmon-user=kirk \
        --title="Captain" \
        --generate-key \
        --group=nosuchgroup

    user_name=$(s9s user --list kirk)
    if [ "$user_name" ]; then
        failure "User created when the group was invalid."
        return 1
    fi

    return 0
}

function testCreateUsers()
{
    #
    # Let's add some users so that we have something to work on.
    #
    mys9s user \
        --create \
        --cmon-user="sisko" \
        --title="Captain" \
        --first-name="Benjamin" \
        --last-name="Sisko"   \
        --email-address="sisko@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
      
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="odo" \
        --first-name="Odo" \
        --last-name="" \
        --email-address="odo@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="jake"\
        --first-name="Jake"\
        --last-name="Sisko"\
        --email-address="jake.sisko@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="bashir" \
        --title="Dr." \
        --first-name="Julian" \
        --last-name="Bashir" \
        --email-address="drbashir@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="chief" \
        --title="Chief" \
        --first-name="Miles" \
        --last-name="O'Brien" \
        --email-address="chief@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="nerys"  \
        --title="Major" \
        --first-name="Kira" \
        --last-name="Nerys" \
        --email-address="kira@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="quark" \
        --first-name="Quark" \
        --last-name=""\
        --email-address="quark@ferengi.fr" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="jadzia" \
        --title="Lt." \
        --first-name="Jadzia" \
        --last-name="Dax"\
        --email-address="dax@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user="worf"\
        --title="Lt." \
        --first-name="Worf" \
        --last-name="" \
        --email-address="warrior@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    s9s user --list --long
}

#
# Running the requested tests.
#
startTests

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    #runFunctionalTest testPing
    runFunctionalTest testGrantUser
    runFunctionalTest testSystemUsers
    runFunctionalTest testFailNoGroup
    runFunctionalTest testCreateUsers
fi

endTests


