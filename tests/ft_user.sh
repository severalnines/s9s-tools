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
CONTAINER_SERVER=""

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
    print_title "Pinging controller."

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

    #
    # 
    #
    print_title "Testing --whoami"

    mys9s user --whoami
    myself=$(s9s user --whoami)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with public key ($myself)"
    else
        printVerbose "   myself : '$myself'"
    fi

    #
    # Checking that we can see the keys.
    #
    if ! s9s user --list-keys | grep -q "Total: 2"; then
        failure "Could not read keys for '$userName'"
        mys9s user --list-keys
        return 1
    fi
}

#
# Using the --stat option on a user.
#
function testStat()
{
    local userName="$USER"
    local lines

    #
    #
    #
    print_title "Testing the --stat option"
    lines=$(s9s user --stat "$userName")

    mys9s user --stat "$userName"

    if ! echo "$lines" | grep -q "Name: $userName"; then
        failure "Username was not found in --stat."
        return 1
    fi
    
    if ! echo "$lines" | grep -q "Class: CmonUser"; then
        failure "Class is not right in --stat."
        return 1
    fi
    
    if ! echo "$lines" | grep -q "ACL: rwxr--r--"; then
        failure "ACL is not right in --stat."
        return 1
    fi
    
    if ! echo "$lines" | grep -q "CDT path: / "; then
        failure "CDT path is not right in --stat."
        return 1
    fi
    
    if ! echo "$lines" | grep -q "Owner: pipas/admins"; then
        failure "Owner is not right in --stat."
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

    print_title "Testing system users"
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

    print_title "Testing wrong password response"

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
        return 1
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

    print_title "Testing missing group response"

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

    print_title "Testing Creation of Users"
    cat <<EOF
This test will create several users through RPC v2 and check if they are
properly created.
EOF

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
        --new-password="secret" \
        --group=ds9 \
        --create-group \
        --batch \
        "sisko"
      
    check_exit_code_no_job $?
    check_user \
        --group      "ds9" \
        --user-name  "sisko" \
        --password   "secret" \
        --email      "sisko@ds9.com" \
        --check-key 

    # User odo
    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --first-name="Odo" \
        --last-name="" \
        --email-address="odo@ds9.com" \
        --generate-key \
        --new-password="odo" \
        --group=ds9 \
        --create-group \
        --batch \
        "odo"
    
    check_exit_code_no_job $?
    check_user \
        --group      "ds9" \
        --user-name  "odo" \
        --password   "odo" \
        --email      "odo@ds9.com" \
        --check-key 

    # User jake
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
    
    check_exit_code_no_job $?

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
    
    check_exit_code_no_job $?

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
    
    check_exit_code_no_job $?

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
    
    check_exit_code_no_job $?

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
    
    check_exit_code_no_job $?

    #
    # After creating all these users the logged in user should still be me.
    #
    myself=$(s9s user --whoami)
    if [ "$myself" != "$USER" ]; then
        failure "The logged in user should be '$USER' instead of '$myself'."
        return 1
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

    print_title "Testing --set option"

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
        return 1
    fi

    emailAddress=$(s9s user --list --user-format="%M" system)
    if [ "$emailAddress" != "system@mydomain.com" ]; then
        failure "The email address is ${emailAddress} instead of 'system@mydomain.com'."
        return 1
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
        return 1
    fi

    emailAddress=$(s9s user --list --user-format="%M" system)
    if [ "$emailAddress" != "system@mynewdomain.com" ]; then
        failure "The email address is ${emailAddress} instead of 'system@mynewdomain.com'."
        return 1
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

    print_title "Testing --set on other user"

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
        return 1
    fi

    emailAddress=$(s9s user --list --user-format="%M" $userName)
    if [ "$emailAddress" != "nobody@mydomain.com" ]; then
        failure "The email is ${emailAddress} instead of 'nobody@mydomain.com'."
        return 1
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
        return 1
    fi

    emailAddress=$(s9s user --list --user-format="%M" $userName)
    if [ "$emailAddress" != "nobody@mynewdomain.com" ]; then
        failure "The email is ${emailAddress} and not 'nobody@mynewdomain.com'."
        return 1
    fi

    return 0
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

    print_title "Testing changing of password"

    #
    # The 'system' user changes the password for nobody.
    #
    mys9s user \
        --change-password \
        --cmon-user="system" \
        --password="secret" \
        --new-password="p" \
        "$userName" 
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
        return 1
    fi

    myself=$(s9s user --whoami --cmon-user=$userName --password=p)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with password ($myself)"
        return 1
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
        return 1
    fi

    myself=$(s9s user --whoami --cmon-user=$userName --password=pp)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with password ($myself)"
        return 1
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

    print_title "Testing keys"

    #
    # We are re-using keys here created in some previous test, so we check if
    # the files exists.
    #
    if [ ! -f $publicKey ]; then
        failure "File '$publicKey' not found."
        return 1
    fi

    if [ ! -f $privateKey ]; then
        failure "File '$rivateKey' not found."
        return 1
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
        $userName 

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while adding key."
        return 1
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
        failure "Failed to log in with public key"
        mys9s user \
            --whoami \
            --cmon-user=$userName \
            --private-key-file=$privateKey
        return 1
    else
        printVerbose "   myself : '$myself'"
    fi 
}

#
# Sets the primary group for a user.
#
function testSetGroup()
{
    local user_name="$USER"
    local group_name="admins"
    local actual_group_name

    #
    #
    #
    print_title "Changing the Primary Group"
    cat <<EOF
This test will change the $user_name user, set its primary group to $group_name
by calling the s9s with the --set-group command line option.
EOF
    mys9s user \
        --set-group \
        --group=admins \
        --cmon-user=system \
        --password=secret \
        pipas

    check_exit_code_no_job $?

    actual_group_name=$(get_user_group "$user_name")
    if [ "$actual_group_name" != "$group_name" ]; then
        failure "The group for '$user_name' is '$actual_group_name'."
    fi

    mys9s user --list --long
    mys9s user --stat "$USER"
}

function testAcl()
{
    local acl

    print_title "Testing ACL on a User"

    mys9s tree --add-acl --acl="user:${USER}:rwx" /sisko
    check_exit_code_no_job $?

    acl=$(s9s tree --list --long sisko --batch | awk '{print $1}')
    if [ "$acl" != "urwxr--r--+" ]; then
        failure "ACL is '$acl' instead of 'urwxr--r--+'."
    fi

    acl=$(s9s user --stat sisko | grep ACL | awk '{print $4}')
    if [ "$acl" != "rwxr--r--+" ]; then
        failure "ACL is '$acl' instead of 'rwxr--r--+'."
    else
        echo "  o ACL in short is '$acl', ok"
    fi
}

function testAddToGroup()
{
    local user_name="sisko"
    local return_code
    local tmp

    print_title "Adding User to a Group"
    cat <<EOF
This test will add a user to a group, so that the user will belong to two
groups after the call. Then the privileges will be checked to see if the
second group is considered when checking access rights.
EOF

    #
    # Checking the access rights.
    #
    mys9s tree \
        --access \
        --privileges="rwx" \
        --cmon-user=sisko \
        /groups

    if [ $? -eq 0 ]; then
        failure "The user sisko has full access to the directory."
    fi

    #
    # Adding the user to the admins group.
    #
    mys9s user \
        --add-to-group \
        --group=admins \
        "$user_name"

    check_exit_code_no_job $?

    tmp=$(get_user_group "$user_name")
    if [ "$tmp" != "ds9,admins" ]; then
        failure "The group is '$tmp' instead of 'ds9,admins'."
    else
        echo "  o group is now 'ds9,admins', ok"
    fi

    #
    # Checking the access rights.
    #
    mys9s tree \
        --access \
        --privileges="rwx" \
        --cmon-user=sisko \
        /groups

    if [ $? -ne 0 ]; then
        failure "The user sisko has full access to the directory."
    fi

    mys9s user --stat sisko

    #
    # Adding the user again should fail.
    #
    print_title "Trying to Add to the Group Again"
    
    mys9s user \
        --add-to-group \
        --group=admins \
        "$user_name"

    return_code=$?

    if [ $return_code -eq 0 ]; then
        failure "Adding to the same group should not be successful."
    elif [ $return_code -ne 2 ]; then
        failure "Adding to the same group should give 2 as exit code."
    else
        echo "  o adding to the same group failed, ok"
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
    
    mys9s user --list --long
else
    runFunctionalTest testUser
    runFunctionalTest testStat
    runFunctionalTest testSetUser
    runFunctionalTest testSetOtherUser
    runFunctionalTest testSystemUsers
    runFunctionalTest testFailNoGroup
    runFunctionalTest testFailWrongPassword
    runFunctionalTest testCreateUsers
    runFunctionalTest testChangePassword
    runFunctionalTest testPrivateKey
    runFunctionalTest testSetGroup
    runFunctionalTest testAcl
    runFunctionalTest testAddToGroup

    print_title "Finished"
    mys9s user --list --long
fi

endTests


