#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.3"
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
    echo "The s9s program is not installed."
    exit 7
fi

#CLUSTER_ID=$($S9S cluster --list --long --batch | awk '{print $1}')

#
# This function is used to cut one cell (of a given row and column) from a table
# formatted text.
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
# Pinging the controller without authenticating.
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
        $OPTION_VERBOSE >/dev/null

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while pinging controller."
        pip-say "The controller is off line. Further testing is not possible."
    else
        pip-say "The controller is on line."
    fi
}

#
# Checking that the current user (created in grant_user()) can log in and can
# view its own public key.
#
function testUser()
{
    local userName="$USER"
    local myself

    myself=$(s9s user --whoami)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with public key ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi

    #
    # Checking that we can see the keys.
    #
    if ! s9s user --list-keys | grep -q "Total: 1"; then
        failure "Could not read keys for '$userName'"
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
# This test will check what happens if using a wrong username or a wrong
# password. The proper exit code and error message is checked.
#
function testFailWrongPassword()
{
    local output
    local exitCode

    #
    # Using the wrong password.
    #
    output=$(s9s user --whoami --cmon-user=system --password=wrongone 2>&1)
    exitCode=$?
    if [ "$exitCode" -ne 3 ]; then
        failure "The exit code is ${exitCode} using a wrong password"
    fi

    #if [ "$output" != "Wrong username or password." ]; then
    #    failure "Wrong error message when using the wrong password"
    #    echo "  output: '$output'"
    #fi
    
    #
    # Using the wrong username.
    #
    output=$(s9s user --whoami --cmon-user=sys --password=secret 2>&1)
    exitCode=$?
    if [ "$exitCode" -ne 3 ]; then
        failure "The exit code is ${exitCode} using a wrong username"
    fi

    #if [ "$output" != "Wrong username or password." ]; then
    #    failure "Wrong error message when using the wrong username"
    #    echo "  output: '$output'"
    #fi
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
        --title="Captain" \
        --generate-key \
        --group=nosuchgroup \
        --batch \
        "kirk"

    user_name=$(s9s user --list kirk 2>/dev/null)
    if [ "$user_name" ]; then
        failure "User created when the group was invalid."
        return 1
    fi

    return 0
}

#
# Creating a bunch of users through the pipe without authentication.
#
function testCreateUsers()
{
    local myself

    #
    # Let's add some users so that we have something to work on.
    #
    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Captain" \
        --first-name="Benjamin" \
        --last-name="Sisko"   \
        --email-address="sisko@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "sisko"
      
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --first-name="Odo" \
        --last-name="" \
        --email-address="odo@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "odo"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --first-name="Jake"\
        --last-name="Sisko"\
        --email-address="jake.sisko@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "jake"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Dr." \
        --first-name="Julian" \
        --last-name="Bashir" \
        --email-address="drbashir@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "bashir"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Chief" \
        --first-name="Miles" \
        --last-name="O'Brien" \
        --email-address="chief@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "chief"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Lt." \
        --first-name="Jadzia" \
        --last-name="Dax"\
        --email-address="dax@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "jadzia"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Lt." \
        --first-name="Worf" \
        --last-name="" \
        --email-address="warrior@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "worf"
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user"
    fi

    #s9s user --list --long

    #
    # After creating all these users the logged in user should still be me.
    #
    myself=$(s9s user --whoami)
    if [ "$myself" != "$USER" ]; then
        failure "The logged in user should be '$USER' instead of '$myself'."
    fi

    return 0
}

#
# This test will change some properties of the same user (the user changes
# itself) and check if the change registered.
#
function testSetUser()
{
    local emailAddress
    local exitCode

    #
    # Setting the email address for a user and checking if it set.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --batch \
        --email-address=system@mydomain.com 

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while changing user"
    fi

    emailAddress=$(s9s user --list --user-format="%M" system)
    if [ "$emailAddress" != "system@mydomain.com" ]; then
        failure "The email address is ${emailAddress} instead of 'system@mydomain.com'."
    fi

    #
    # Setting the email address again.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --batch \
        --email-address=system@mynewdomain.com 

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while changing user"
    fi

    emailAddress=$(s9s user --list --user-format="%M" system)
    if [ "$emailAddress" != "system@mynewdomain.com" ]; then
        failure "The email address is ${emailAddress} instead of 'system@mynewdomain.com'."
    fi

    return 0
}

#
# This test will change some properties of some user (the user changes
# some other user) and check if the change registered.
#
function testSetOtherUser()
{
    local userName="nobody"
    local emailAddress
    local exitCode

    #
    # Setting the email address for a user and checking if it set.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --batch \
        --email-address=nobody@mydomain.com \
        "$userName"

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while changing user"
    fi

    emailAddress=$(s9s user --list --user-format="%M" $userName)
    if [ "$emailAddress" != "nobody@mydomain.com" ]; then
        failure "The email is ${emailAddress} instead of 'nobody@mydomain.com'."
    fi

    #
    # Setting the email address again.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --batch \
        --email-address=nobody@mynewdomain.com \
        "$userName"

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while changing user"
    fi

    emailAddress=$(s9s user --list --user-format="%M" $userName)
    if [ "$emailAddress" != "nobody@mynewdomain.com" ]; then
        failure "The email is ${emailAddress} and not 'nobody@mynewdomain.com'."
    fi

    return 0
}

#
# This test will create a new user through the RPC v2 encrypted network
# connecttion (instead of using the old named pipe connection). We are creating
# the new user, a new group, RSA keys and also a password, then we test if we
# can login with the keypair and also the password.
#
function testCreateThroughRpc()
{
    local newUserName="rpc_user"
    local userId
    local myself

    #
    # Here we pass the --cmon-user and --password options when creating the new
    # user, so the client will try to send the createUser request to RPC v2
    # through the network and not through the named pipe.
    #
    mys9s user \
        --create \
        --cmon-user="system" \
        --password="secret" \
        --group="rpc_group" \
        --create-group \
        --email-address="rpc@email.com" \
        --generate-key \
        --new-password="p" \
        "$newUserName" >/dev/null

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
    fi

    #
    # Checking some properties.
    #
    userId=$(s9s user --list --user-format="%I" $newUserName)
    if [ "$userId" -gt 0 ]; then
        printVerbose "  user_id : $userId"
    else
        failure "The user ID is invalid while creating user through RPC"
    fi
    
    group=$(s9s user --list --user-format="%G" $newUserName)
    if [ "$group" == "rpc_group" ]; then
        printVerbose "    group : '$group'"
    else
        failure "The group is '$group' while creating user through RPC"
    fi
    
    email=$(s9s user --list --user-format="%M" $newUserName)
    if [ "$email" == "rpc@email.com" ]; then
        printVerbose "    email : '$email'"
    else
        failure "The email is '$email' while creating user through RPC"
    fi

    #
    # Testing if we can log in with the shiny new password.
    #
    myself=$(s9s user --whoami --cmon-user=rpc_user --password=p)
    if [ "$myself" != "rpc_user" ]; then
        failure "Failed to log in with password ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi

    #
    # Then we check if the key files are created and will try to log in using
    # the RSA key. Well, we are not passing the --password option so the s9s
    # client will try to log in with the key.
    #
    file="$HOME/.s9s/rpc_user.key"
    if [ ! -f "$file" ]; then
        failure "File '$file' was not created"
    fi
    
    file="$HOME/.s9s/rpc_user.pub"
    if [ ! -f "$file" ]; then
        failure "File '$file' was not created"
    fi
    
    myself=$(s9s user --whoami --cmon-user=rpc_user)
    if [ "$myself" != "rpc_user" ]; then
        failure "Failed to log in with password ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi

    #
    # Checking that we can see the keys.
    #
    if ! s9s user --list-keys --cmon-user=rpc_user | grep -q "Total: 1"; then
        failure "Could not read keys for 'rpc_user'"
    fi
}

#
# This test will try to change the password for a user. First a user changes the
# password for an other user, then this other user uses the new password for
# changing his own password again. Classic... :)
#
function testChangePassword()
{
    local userName="nobody"
    local myself

    #
    # The 'system' user changes the password for nobody.
    #
    mys9s user \
        --change-password \
        --cmon-user="system" \
        --password="secret" \
        --new-password="p" \
        "$userName" \
        >/dev/null 
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
    fi

    myself=$(s9s user --whoami --cmon-user=$userName --password=p)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with password ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi
    
    #
    # Nobody uses this new password to change the password again.
    #
    mys9s user \
        --change-password \
        --cmon-user="$userName" \
        --password="p" \
        --new-password="pp" \
        >/dev/null
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
    fi

    myself=$(s9s user --whoami --cmon-user=$userName --password=pp)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with password ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi
}

#
# Registering a public key and using its private counterpart to authenticate.
#
function testPrivateKey()
{
    local userName="worf"
    local publicKey="$HOME/.s9s/kirk.pub"
    local privateKey="$HOME/.s9s/kirk.key"
    local myself

    #
    # We are re-using keys here created in some previous test, so we check if
    # the files exists.
    #
    if [ ! -f $publicKey ]; then
        failure "File '$publicKey' not found."
        return
    fi

    if [ ! -f $privateKey ]; then
        failure "File '$rivateKey' not found."
        return
    fi

    #
    # Registering a new key, checking the exitcode.
    #
    mys9s user \
        --cmon-user=system \
        --password=secret \
        --add-key \
        --public-key-file=$publicKey \
        --public-key-name="mykeyfile" \
        $userName \ >/dev/null 2>/dev/null 

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while adding key."
    else
        printVerbose "Key file '$publicKey' registered."
    fi

    #
    # Authenticating with the private counterpart.
    #
    myself=$(\
        s9s user \
            --whoami \
            --cmon-user=$userName \
            --private-key-file=$privateKey)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with public key ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi
    
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
    
    s9s user --list --long
else
    #runFunctionalTest testPing
    runFunctionalTest testUser
    runFunctionalTest testSetUser
    runFunctionalTest testSetOtherUser
    runFunctionalTest testSystemUsers
    runFunctionalTest testFailNoGroup
    runFunctionalTest testFailWrongPassword
    runFunctionalTest testCreateUsers
    runFunctionalTest testCreateThroughRpc
    runFunctionalTest testChangePassword
    runFunctionalTest testPrivateKey
    
    #s9s user --list --long
fi

endTests


