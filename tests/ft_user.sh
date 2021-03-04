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

#OPTION_CMON_PORT="9501"

cd $MYDIR
source include.sh
source shared_test_cases.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]... [TESTNAME]

  $MYNAME - Test script for s9s to check various error conditions.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --force-sqlite   Use SQLite despite the setting in the config files.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,reset-config,server:,\
force-sqlite" \
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
    cat <<EOF | paragraph
Checking that the authenticated user is what it is expected and it can reach
information about itself. Checking if the keys are ok.
EOF

    begin_verbatim
    mys9s user --whoami
    myself=$(s9s user --whoami)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with public key ($myself)"
    else
        success "  o returned '$myself', ok"
    fi

    #
    # Checking that we can see the keys.
    #
    if ! s9s user --list-keys | grep -q "Total: 2"; then
        failure "Could not read keys for '$userName'"
        mys9s user --list-keys
        return 1
    else
        success "  o the keys are there, ok"
    fi

    end_verbatim
}

function testUserManager()
{
    local userName="$USER"
    print_title "Giving the Right to Create Users"
    cat <<EOF | paragraph
Giving the user '$userName' the right to create other users.
EOF

    begin_verbatim
    mys9s tree \
        --add-acl \
        --cmon-user="system" \
        --password="secret" \
        --acl="user:${userName}:rwx" \
        /.runtime/user_manager

    check_exit_code_no_job $?
    end_verbatim
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

    begin_verbatim
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
        failure "Owner is not 'pipas/admin' in --stat."
        return 1
    fi

    end_verbatim
}

function testTree()
{
    print_title "Testing the Tree"

    begin_verbatim
    mys9s tree --list --long --recursive --full-path
    end_verbatim

}

function testCmonGroup()
{
    local old_ifs="$IFS"
    local found_names=0

    print_title "Testing CmonGroup Class"
    cat <<EOF
This test will list the properties of the CmonGroup class and check that the
property list is indeed the same as we expect.
EOF

    begin_verbatim
    mys9s metatype --list-properties --type=CmonGroup --long

    IFS=$'\n'
    for line in $(s9s metatype --list-properties --type=CmonGroup --long --batch)
    do
        name=$(echo "$line" | awk '{print $2}')
        mode=$(echo "$line" | awk '{print $1}')

        case "$name" in 
            acl)
                let found_names+=1
                ;;

            cdt_path)
                let found_names+=1
                ;;

            created)
                let found_names+=1
                ;;

            group_id)
                let found_names+=1
                ;;

            group_name)
                let found_names+=1
                ;;

            owner_group_id)
                let found_names+=1
                ;;

            owner_group_name)
                let found_names+=1
                ;;

            owner_user_id)
                let found_names+=1
                ;;

            owner_user_name)
                let found_names+=1
                ;;
            
            tags)
                let found_names+=1
                ;;

            *)
                failure "Unidentified property '$name'."
        esac
    done
    IFS=$old_ifs

    if [ "$found_names" -lt 9 ]; then
        failure "Some property names were not found."
    else
        success "  o found $found_names properties, ok"
    fi

    end_verbatim
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

    print_title "Testing System Users"
    cat <<EOF
This test will go through the users that are always present (system users) and
check that their properties are as they are expected.
EOF

    # This command 
    #$S9S user --list --long 
    begin_verbatim
    text=$($S9S user --list --long --batch --color=never)

    row=1
    column=2
    cell=$(index_table "$text" $row $column)
    required="1"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        mys9s user --list --long --batch
        return 1
    fi

    row=1
    column=3
    cell=$(index_table "$text" $row $column)
    required="system"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        mys9s user --list --long --batch
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
        mys9s user --list --long --batch
        return 1
    fi
    
    row=2
    column=3
    cell=$(index_table "$text" $row $column)
    required="nobody"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        mys9s user --list --long --batch
        return 1
    fi
    
    row=2
    column=4
    cell=$(index_table "$text" $row $column)
    required="nobody"
    if [ "$cell" != "$required" ]; then
        failure "The cell at $row,$column should be '$required', it is '$cell'"
        mys9s user --list --long --batch
        return 1
    fi

    #
    #
    #
    check_user \
        --user-name  "system" \
        --group      "admins" \
        --password   "secret" \
        --email      "system@mynewdomain.com" 
    
    check_user \
        --user-name  "nobody" \
        --group      "nobody" 
    
    check_user \
        --user-name  "admin" \
        --group      "admins" 

    end_verbatim
}

#
# This test will check what happens if using a wrong username or a wrong
# password. The proper exit code and error message is checked.
#
function testFailWrongPassword()
{
    local output
    local exitCode
    local expected

    #
    # Using the wrong password.
    #
    print_title "Trying a Wrong Password"
    cat <<EOF | paragraph
  This test will try to authenticate the system user with a wrong password
  and check that both the return value and the error message shows the failure.
  Then the test checks that the failed attempty is indicated in both the user
  stats and the logs.
EOF

    begin_verbatim
    mys9s user --whoami --cmon-user=system --password=wrongone

    output=$(s9s user --whoami --cmon-user=system --password=wrongone 2>&1)
    exitCode=$?
    if [ "$exitCode" -ne 3 ]; then
        failure "The exit code is ${exitCode} using a wrong password."
    else
        success "  o exit code is $exitCode, ok"
    fi

    expected="Access denied."
    if [ "$output" != "$expected" ]; then
        failure "Wrong error message when using the wrong password."
        failure "    output: '$output'"
        failure "  expected: '$expected'"
    else
        success "  o output is '$output', ok"
    fi

    # Checking the user stats.
    output=$(s9s user --stat system | grep Failure | awk '{print $2}')
    if [ "$output" == "-" ]; then
        failure "Failed login is not indicated."
    else
        success "  o failed login at '$output', ok"
    fi
   
    # Checking the logs.
    check_log_messages \
        "failed to log in with password"
    
    end_verbatim

    #
    # Using the wrong username.
    #
    print_subtitle "Trying a Wrong Username"

    begin_verbatim
    output=$(s9s user --whoami --cmon-user=sys --password=secret 2>&1)
    exitCode=$?
    if [ "$exitCode" -ne 3 ]; then
        failure "The exit code is ${exitCode} using a wrong username."
    else
        success "  o exit code is $exitCode, ok"
    fi

    expected="Username or password is incorrect."
    if [ "$output" != "$expected" ]; then
        failure "Wrong error message when using the wrong username."
        failure "    output: '$output'"
        failure "  expected: '$expected'"
    else
        success "  o output is '$output', ok"
    fi
   
    end_verbatim
}

#
# Testing what happens when a creation of a new user fails because the group 
# does not exist.
#
function testFailNoGroup()
{
    local user_name

    print_title "Creating User without Group"

    begin_verbatim
    #
    # Yes, this is a problem, we can't get an error message back from the pipe.
    # The group here does not exists and we did not request the creation of the
    # group, so this will fail, but we still get the AOK back from the program.
    #
    mys9s user \
        --create \
        --cmon-user=system \
        --password=secret \
        --title="Captain" \
        --generate-key \
        --group=nosuchgroup \
        "kirk"

    user_name=$(s9s user --list kirk 2>/dev/null)
    if [ "$user_name" ]; then
        failure "User created when the group was invalid."
        return 1
    else
        success "  o user was not created, ok"
    fi

    S9S_LAST_OUTPUT_CONTAINS \
        "Group 'nosuchgroup' does not exists."

    #
    # Creating the user with no group. The user will end up in the "users"
    # group.
    #
    mys9s user \
        --create \
        --title="Captain" \
        --generate-key \
        --create-group \
        --first-name="James" \
        --last-name="Kirk"   \
        --email-address="kirk@enterprise.com" \
        --new-password="secret" \
        --cmon-user=system \
        --password=secret \
        "kirk"
    
    check_exit_code_no_job $?
   
    check_user \
        --group        "users" \
        --user-name    "kirk"  \
        --password     "secret" \
        --email        "kirk@enterprise.com"
    
    end_verbatim
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

    begin_verbatim
    #
    # Let's add some users so that we have something to work on.
    #

    # This is where we create a new group ds9 with a new user.
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

    check_group \
        --group-name   "ds9"     \
        --owner-name   "system"  \
        --group-owner  "admins" 
  
    check_user \
        --group      "ds9" \
        --user-name  "sisko" \
        --password   "secret" \
        --email      "sisko@ds9.com" \
        --check-key 
    
    check_log_messages \
        "Creating new cmon user 'sisko'"

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
    
    check_log_messages \
        "Creating new cmon user 'odo'"

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
    
    check_log_messages \
        "Creating new cmon user 'jake'"

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
    
    check_log_messages \
        "Creating new cmon user 'bashir'"

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
    
    check_log_messages \
        "Creating new cmon user 'chief'"

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
    
    check_log_messages \
        "Creating new cmon user 'jadzia'"

    #
    # Trying to create a user as normal user (this feature was removed) and then
    # as system user (this should succeed of course.
    #
    print_subtitle "Creating user 'worf'"
    mys9s user \
        --create \
        --title="Lt." \
        --first-name="Worf" \
        --last-name="" \
        --email-address="warrior@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "worf"
  
    retcode=$?
    if [ $retcode == 0 ]; then
        failure "Normal user should not be able to create new users (retcode = $retcode)."
        mys9s user --list --long
    else
        success "  o Normal user can not create new user, OK"
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

    check_exit_code_no_job $?

    mys9s tree --get-acl /worf
    mys9s user --stat worf
    mys9s user --list --long

    #mys9s user --stat worf
    #mys9s user --whoami --print-json --cmon-user worf

    #
    # After creating all these users the logged in user should still be me.
    #
    myself=$(s9s user --whoami)
    if [ "$myself" != "$USER" ]; then
        failure "The logged in user should be '$USER' instead of '$myself'."
        return 1
    fi

    end_verbatim
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

    begin_verbatim
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

    end_verbatim
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

    print_title "Testing --set on Other User"

    begin_verbatim

    #
    # Setting the email address for a user and checking if it set.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --email-address=nobody@mydomain.com \
        "$userName"

    check_exit_code_no_job $?

    emailAddress=$(s9s user --list --user-format="%M" $userName)
    if [ "$emailAddress" != "nobody@mydomain.com" ]; then
        failure "The email is ${emailAddress} instead of 'nobody@mydomain.com'."
    else
        success "  o The email address has been changed, ok."
    fi

    #
    # Setting the email address again.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --email-address=nobody@mynewdomain.com \
        "$userName"

    check_exit_code_no_job $?

    emailAddress=$(s9s user --list --user-format="%M" $userName)
    if [ "$emailAddress" != "nobody@mynewdomain.com" ]; then
        failure "The email is ${emailAddress} and not 'nobody@mynewdomain.com'."
    else
        success "  o The email is  ${emailAddress}, ok."
    fi

    #
    # Setting an email address for a user that does not exist.
    #
    mys9s user \
        --set \
        --cmon-user=system \
        --password=secret \
        --batch \
        --email-address=nobody@mynewdomain.com \
        "nonexistinguser"

    exitCode=$?
    if [ "$exitCode" == 0 ]; then
        failure "The exit code should not be 0 for a non existing user."
    else
        success "  o Exit code is $exitCode, ok."
    fi

    end_verbatim
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

    print_title "Testing the Changing of the Password"

    begin_verbatim

    #
    # The 'system' user changes the password for nobody. There is no old
    # password passed here.
    #
    mys9s user \
        --change-password \
        --cmon-user="system" \
        --password="secret" \
        --new-password="p" \
        "$userName" 
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while changing password."
        return 1
    else
        success "  o Changing password succeeded, ok."
    fi

    myself=$(s9s user --whoami --cmon-user=$userName --password=p)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with password ($myself)"
        return 1
    else
        success "  o The new password works, ok."
    fi
    
    #
    # User 'nobody' uses this new password to change the password again.
    #
    mys9s user \
        --change-password \
        --cmon-user="$userName" \
        --password="p" \
        --new-password="pp" 
    
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while changing the password."
    else
        success "  o The new password works, ok."
    fi

    myself=$(s9s user --whoami --cmon-user=$userName --password=pp)
    if [ "$myself" != "$userName" ]; then
        failure "Failed to log in with password ($myself)."
    else
        success "  o The second password works, ok."
    fi

    end_verbatim
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

    begin_verbatim
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

    end_verbatim
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
  This test will change the $user_name user, set its primary group to
  $group_name by calling the s9s with the --set-group command line option.
  
EOF

    begin_verbatim
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
    else
        success "  o The group for '$user_name' is '$actual_group_name', ok."
    fi

    mys9s user --list --long
    mys9s user --stat "$USER"
    end_verbatim
}

function testAcl()
{
    local acl

    print_title "Testing ACL on a User"
    cat <<EOF
This test will add an ACL entry to a user (changing the accessibility of the 
given user) and check if the ACL is properly printed in the stat page of the
user object.
EOF

    begin_verbatim
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
        success "  o ACL in short is '$acl', ok"
    fi

    end_verbatim
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

    begin_verbatim
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
        success "  o group is now 'ds9,admins', ok"
    fi

    #
    # Checking the access rights.
    #
    mys9s tree \
        --access \
        --privileges="rwx" \
        --cmon-user="$user_name" \
        /groups

    if [ $? -ne 0 ]; then
        failure "The user sisko has full access to the directory."
    fi

    mys9s user --stat sisko
    #print_log_messages
    end_verbatim

    #
    # Adding the user again should fail.
    #
    print_subtitle "Trying to Add to the Group Again"
    cat <<EOF
  Checking if adding the user to a group it already belongs to will result in 
  an error.
EOF

    begin_verbatim
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
        success "  o adding to the same group failed, ok"
    fi

    #print_log_messages
    end_verbatim

    #
    # Removing the user from the group.
    #
    print_subtitle "Removing the User from the Group"
    cat <<EOF
Here we remove the user $user_name from the admins group and check if the user
was indeed removed and the special privileges are revoked.
EOF

    begin_verbatim
    mys9s user \
        --remove-from-group \
        --group=admins \
        "$user_name"
    
    mys9s tree \
        --access \
        --privileges="rwx" \
        --cmon-user="$user_name" \
        /groups

    if [ $? -eq 0 ]; then
        failure "The user sisko has full access to the directory."
    else
        success "  o user lost access, ok"
    fi
    
    tmp=$(get_user_group "$user_name")
    if [ "$tmp" != "ds9" ]; then
        failure "The group is '$tmp' instead of 'ds9'."
    else
        success "  o group is now 'ds9', ok"
    fi

    end_verbatim
}

function check_extended_privileges()
{
    local cmon_user_option=""
    local can_execute_job=""
    local can_create_cluster=""
    local retcode
    local tmp

    while [ -n "$1" ]; do
        case "$1" in
            --cmon-user)
                cmon_user_option=" --cmon-user=$2";
                shift 2
                ;;

            --can-execute-job)
                can_execute_job="true"
                shift
                ;;

            --can-create-cluster)
                can_create_cluster="true"
                shift
                ;;
        esac
    done

    # The file privileges.
    mys9s tree$cmon_user_option \
        --access --privileges="r-x" \
        /.runtime/jobs/jobExecutor

    retcode=$?
    if [ -n "$can_execute_job" ]; then
        if [ "$retcode" -eq 0 ]; then
            success "  o The user can execute jobs, ok."
        else
            failure "The user should be able to execute jobs."
        fi
    else
        if [ "$retcode" -eq 0 ]; then
            failure "The user should not be able to execute jobs."
        else
            success "  o The user is not able to execute jobs, ok."
        fi
    fi
    
    mys9s tree$cmon_user_option \
        --access --privileges="r-x" \
        /.runtime/jobs/jobExecutorCreateCluster
    
    retcode=$?
    if [ -n "$can_execute_job" ]; then
        if [ "$retcode" -eq 0 ]; then
            success "  o The user can create clusters, ok."
        else
            failure "The user should be able to create clusters."
        fi
    else
        if [ "$retcode" -eq 0 ]; then
            failure "The user should not be able to create clusters."
        else
            success "  o The user can not create clusters, ok."
        fi
    fi

    # The same information in the RPC reply.
    echo -e "$TERM_NORMAL"

    my_command s9s user$cmon_user_option --whoami --print-json \
        \| jq .user_extended_privileges

    s9s user$cmon_user_option --whoami --print-json | \
        jq .user_extended_privileges

    tmp=$(s9s user$cmon_user_option --whoami --print-json |\
        jq .user_extended_privileges.can_execute_jobs)
    if [ "$tmp" == "true" ]; then
        if [ "$retcode" -eq 0 ]; then
            success "  o The user can execute jobs, ok."
        else
            failure "The user should be able to execute jobs."
        fi
    else
        if [ "$retcode" -eq 0 ]; then
            failure "The user should be not able to execute jobs."
        else
            success "  o The user can not execute jobs, ok."
        fi
    fi

    tmp=$(s9s user$cmon_user_option --whoami --print-json |\
        jq .user_extended_privileges.can_create_clusters)
    if [ "$tmp" == "true" ]; then
        if [ "$retcode" -eq 0 ]; then
            success "  o The user can create clusters, ok."
        else
            failure "The user should be able to create clusters."
        fi
    else
        if [ "$retcode" -eq 0 ]; then
            failure "The user should not be able to create clusters."
        else
            success "  o The user can not create clusters, ok."
        fi
    fi
}

function checkExtendedPrivileges()
{
    local tmp

    print_title "Checking Extended Privileges"
    cat <<EOF
  This test will check how the extended privileges (can execute a job and can
  create a new cluster) can be controlled by setting ACL entries to some special
  CDT entries in /.runtime/jobs/. The test will check the default values for
  some users, then deny these privileges with a user ACL entry and a group ACL
  entry.

EOF

    begin_verbatim
    my_command  s9s user --whoami --print-json \| jq .user_extended_privileges
    s9s user --whoami --print-json | jq .user_extended_privileges

    
    #
    # Defaults.
    #
    check_extended_privileges \
        --can-execute-job \
        --can-create-cluster
  
    end_verbatim

    #
    #
    #
    print_subtitle "Denying by User ACL Entry"
    begin_verbatim
    check_extended_privileges \
        --cmon-user          "kirk"  \
        --can-execute-job            \
        --can-create-cluster

    mys9s tree --add-acl --acl="user:kirk:---" \
        /.runtime/jobs/jobExecutor
    
    check_exit_code_no_job $?
    
    mys9s tree --add-acl --acl="user:kirk:---" \
        /.runtime/jobs/jobExecutorCreateCluster

    mys9s tree --get-acl /.runtime/jobs/jobExecutorCreateCluster
    mys9s tree --get-acl /.runtime/jobs/jobExecutorCreateCluster

    check_extended_privileges \
        --cmon-user          "kirk"  

    end_verbatim

    #
    # Denying by group ACL.
    #
    print_subtitle "Denying by a Group ACL Entry"
    begin_verbatim

    mys9s tree \
        --add-acl \
        --acl="other::r-x" \
        "/.runtime/jobs/jobExecutor"
    
    mys9s tree \
        --add-acl \
        --acl="other::r-x" \
        "/.runtime/jobs/jobExecutorCreateCluster"

    check_extended_privileges \
        --cmon-user          "worf"  \
        --can-execute-job            \
        --can-create-cluster
    
    mys9s tree \
        --add-acl \
        --acl="group:ds9:---" \
        /.runtime/jobs/jobExecutor
    
    mys9s tree --add-acl --acl="group:ds9:---" \
        /.runtime/jobs/jobExecutorCreateCluster
    
    mys9s tree --get-acl /.runtime/jobs/jobExecutor
    mys9s tree --get-acl /.runtime/jobs/jobExecutorCreateCluster

    check_extended_privileges \
        --cmon-user          "worf"  

    mys9s tree --access --privileges="r-x" /.runtime/jobs/jobExecutor
    mys9s tree --access --privileges="r-x" /.runtime/jobs/jobExecutorCreateCluster

    tmp=$(s9s user --whoami --print-json | \
        jq .user_extended_privileges.can_execute_jobs)
    if [ "$tmp" == "true" ]; then
        success "  o The user can execute jobs, ok."
    else
        failure "The user should be able to execute jobs."
    fi

    tmp=$(s9s user --whoami --print-json | \
        jq .user_extended_privileges.can_create_clusters)
    if [ "$tmp" == "true" ]; then
        success "  o The user can create clusters, ok."
    else
        failure "The user should be able to create clusters."
    fi

    end_verbatim
}

function testUserSelfAdmin()
{
    local ret_code

    print_title "Testing Security"
    cat <<EOF
  In this test a new admin user is created, then this new admin user creates yet
  another user ("student") that has no superuser privileges. Then this "student"
  user tries to get superuser privileges by adding itself to the admins group.
  This of course should fail, it would be a big security hole indeed.

EOF

    begin_verbatim
    mys9s user \
        --create \
        --generate-key \
        --group="admins" \
        --new-password="admin" \
        --email-address="test@severalnines.com" \
        --first-name="QA" \
        --last-name="s9s" \
        "test"

    check_exit_code_no_job $?
    
    mys9s user \
        --create \
        --cmon-user="test" \
        --generate-key \
        --group="students" \
        --new-password="student" \
        --create-group \
        "student"

    check_exit_code_no_job $?
        
    check_group \
        --group-name   "students"     \
        --owner-name   "test"         \
        --group-owner  "admins" 

    #
    #
    #
    mys9s user \
        --set-group \
        --group=admins \
        --cmon-user="student" \
        student
    
    ret_code=$?
    if [ "$ret_code" == 0 ]; then
        failure "User should not be able to make itself a superuser."
    else
        success "  o User can not make itself an admin user, ok."
    fi
    
    mys9s user \
        --add-to-group \
        --group=admins \
        --cmon-user="student" \
        student
    
    ret_code=$?
    if [ "$ret_code" == 0 ]; then
        failure "User should not be able to make itself a superuser."
    else
        success "  o User can not make itself an admin user, ok."
    fi

    print_subtitle "Checking the \"student\" User"
    mys9s user --stat student

    if s9s user --stat student | grep -q "Groups: students "; then
        success "  o The user is still belongs to the 'students' group, ok."
    else
        failure "The group for 'student' does not look ok."
    fi

    end_verbatim
}

function testWeirdChar()
{
    print_title "Creating a User and a Group With '#'"
    
    begin_verbatim
    mys9s user \
        --create \
        --cmon-user="system" \
        --password="secret" \
        --generate-key \
        --group="Group#1" \
        --new-password="User#1" \
        --create-group \
        "User#1"
    
    check_exit_code_no_job $?

    check_user \
        --group      "Group#1"    \
        --user-name  "User#1"     \
        --password   "User#1"     \
        --check-key 
    
    check_group \
        --group-name   "Group#1"  \
        --owner-name   "system"   \
        --group-owner  "admins" 

    end_verbatim
}

function testDeleteGroup()
{
    print_title "Checking if Groups can be Created and Deleted"

    cat <<EOF
  This test will test the creation and deletion of Cmon Groups. It will try to
  delete groups, check for error conditions and will also check that the groups
  can actually be deleted.

EOF

    begin_verbatim

    #
    #
    #
    mys9s group --delete "nonexisting"
    if [ $? -ne 0 ]; then
        success "  o Can not delete groups that does not exist, ok."
    else
        failure "Deleting nonexisting groups should not be possible."
    fi

    #
    #
    #
    mys9s group --delete "Group#1"
    if [ $? -ne 0 ]; then
        success "  o Can not delete groups that has members, ok."
    else
        failure "Deleting groups with members should not be possible."
    fi

    #
    # Creating a group and immediately deleting it.
    #
    mys9s group --create "tmpgroup"
    if [ $? -eq 0 ]; then
        success "  o New group could be created, ok."
    else
        failure "Group could not be created."
        return 1
    fi
    
    check_group \
        --group-name   "tmpgroup" \
        --owner-name   "pipas"    \
        --group-owner  "admins"
    
    check_log_messages \
        "Creating new cmon group 'tmpgroup'" 
    

    #
    # 
    #
    mys9s group --delete --cmon-user=student --password=student "tmpgroup"
    if [ $? -ne 0 ]; then
        success "  o Group can not be deleted with insufficient rights."
    else
        failure "User 'student' should not be able to delete group 'tmpgroup'."
    fi

    #
    # Successful deletion of the group.
    #
    mys9s group --delete "tmpgroup"
    
    if [ $? -eq 0 ]; then
        success "  o The group could be deleted, ok."
    else
        failure "Group could not be deleted."
        return 1
    fi

    if s9s group --list | grep -q tmpgroup; then
        failure "The group still exists after it is deleted."
    else
        success "  o The group is actually deleted, ok."
    fi

    end_verbatim
}

function checkPasswordReset()
{
    local retCode
    local mailFile

    print_title "Checking Password Reset Option"
    cat <<EOF
  This test will perform a full pasword reset using a token. The new password is
  set and then checked.

EOF
    
    begin_verbatim
    rm -rf "/tmp/cmon/emails/outgoing/"
    mys9s user --password-reset --cmon-user admin 

    retCode=$?
    if [ $retCode -eq 0 ]; then
        failure "This should have failed, the user has no email address."
    else
        success "  o Failed, no email address, ok."
    fi
    
    #
    # Sending the password reset mail.
    #
    mys9s user --password-reset --cmon-user system
    retCode=$?
    
    mailFile="/tmp/cmon/emails/outgoing/system@mynewdomain.com/email_0000.json";
    if [ -f "$mailFile" ]; then
        cat $mailFile | jq .
    fi

    if [ $retCode -eq 0 ]; then
        success "  o Password reset request is succeeded, ok."
    else
        failure "Password reset failed."
    fi

    if [ -f "$mailFile" ]; then
        success "  o The mail is found, ok."
    else
        failure "The mail is not found in '$mailFile'."
    fi
        

    token=$(cat $mailFile | \
        jq .elements[0].values | \
        jq '.["@PASSWORD_RESET_TOKEN@"]')
    if [ -n "$token" ]; then
        success "  o The token is '$token', ok"
    else
        failure "Token not found."
    fi
    
    recipient=$(cat $mailFile | \
        jq .elements[0].values | \
        jq '.["@RECIPIENT@"]')
    if [ -n "$recipient" ]; then
        success "  o The recpient found ('$recipient'), ok"
        if [ "$recipient" == '"system@mynewdomain.com"' ]; then
            success "  o Recipient is correct, ok."
        else
            failure "Recipient should be \"system@mynewdomain.com\"."
        fi
    else
        failure "Recipient not found."
    fi

    #
    # Password reset with the token.
    #
    token=$(echo $token | tr -d '"')

    mys9s user \
        --password-reset \
        --cmon-user=system \
        --token=$token \
        --new-password="newpassword"

    check_exit_code_no_job $?

    mys9s user --whoami --cmon-user="system" --password="newpassword"
    check_exit_code_no_job $?
    
    check_log_messages \
        --cmon-user    "system" \
        --password     "newpassword" \
        "Created password reset token" \
        "Password reset token is validated"

    #print_log_messages
    end_verbatim
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
    runFunctionalTest testPing
    runFunctionalTest testUser
    runFunctionalTest testUserManager
    runFunctionalTest testStat
    runFunctionalTest testTree
    runFunctionalTest testCmonGroup
    runFunctionalTest testSetUser
    runFunctionalTest testSetOtherUser
    runFunctionalTest testCreateOutsider
    runFunctionalTest testSystemUsers
    runFunctionalTest testFailNoGroup
    runFunctionalTest testFailWrongPassword
    runFunctionalTest testCreateUsers
    runFunctionalTest testChangePassword
    runFunctionalTest testPrivateKey
    runFunctionalTest testSetGroup
    runFunctionalTest testAcl
    runFunctionalTest testAddToGroup
    runFunctionalTest checkExtendedPrivileges
    runFunctionalTest testUserSelfAdmin
    runFunctionalTest testWeirdChar
    runFunctionalTest testDeleteGroup
    runFunctionalTest checkPasswordReset
fi

endTests


