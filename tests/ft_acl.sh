#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: 
  $MYNAME [OPTION]...

  $MYNAME - Tests moving objects in the Cmon Directory Tree.

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

if [ -z "$S9S" ]; then
    printError "The s9s program is not installed."
    exit 7
fi

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

function testCreateUsers()
{
    print_title "Creating a User"

    mys9s user \
        --create \
        --cmon-user="system" \
        --password="secret" \
        --group="users" \
        --create-group \
        --email-address="laszlo@severalnines.com" \
        --first-name="Laszlo" \
        --last-name="Pere"   \
        --generate-key \
        --new-password="pipas" \
        "pipas"

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
        exit 1
    fi

    print_title "Creating a Superuser"
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

    #mys9s tree --tree
    return 0
}

function checkTree01()
{
    local expected
    local lines=$(s9s tree --cmon-user=admin --list)

    print_title "Checking Tree"
    mys9s tree --cmon-user=admin --tree
    mys9s tree --cmon-user=admin --tree /groups

    print_title "Checking Tree List"
    mys9s tree --cmon-user=admin --list

    # The root directory owned by the system user.
    expected="^frwxrwxrwx  system admins /$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi

    # The admin user has a link that the admin will see
    expected="^urwxr--r--  admin  admins /admin -> /admin$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi

    # The non-admin user has a link too.
    expected="^urwxr--r--  pipas  admins /pipas -> /pipas$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi
    
    # The user owns itself.
    expected="^urwxr--r--  pipas  admins /pipas$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi

    return 0
}

function checkTree02()
{
    print_title "Checking Access Rights"

    mys9s tree --access --privileges="rwx" --cmon-user="admin" /
    if [ $? -ne 0 ]; then
        failure "User 'admin' ha no access to '/'"
        exit 1
    fi

    mys9s tree --access --privileges="rwx" --cmon-user="pipas" /
    if [ $? -ne 0 ]; then
        failure "User 'pipas' ha no access to '/'"
        exit 1
    fi

    return 0
}

#
# Creating a folder, checking that it is there.
#
function testMkdir()
{
    local exitCode 
    local expected
    local lines

    print_title "Creating a Folder"

    # Creating a folder.
    mys9s tree --mkdir /home

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating a folder"
        exit 1
    fi

    # Checking the new folder.
    mys9s tree --cmon-user=admin --list
    lines=$(s9s tree --cmon-user=admin --list)
    
    expected="^frwxrwxrwx  pipas  users  /home$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi

    return 0
}

#
# Reading and setting ACL entries.
#
function testAcl()
{
    local lines
    local expected

    #
    # Checking the default ACL entries.
    #
    print_title "Checking Default ACL Entries"
    mys9s tree --get-acl /home
    
    lines=$(s9s tree --get-acl /home)
    expected="^user::rwx$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s tree --get-acl /home
        exit 1
    fi
    
    expected="^group::rwx$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s tree --get-acl /home
        exit 1
    fi
    
    expected="^other::rwx$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s tree --get-acl /home
        exit 1
    fi

    # 
    # We are now adding a new ACL to the folder, a real ACL entry with a user.
    #
    print_title "Adding ACL Entry"
    mys9s tree --add-acl --acl="user:nobody:r-x" /home
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while adding ACL entry."
        exit 1
    fi
    
    mys9s tree --get-acl /home
    lines=$(s9s tree --get-acl /home)
    expected="^user:nobody:r-x$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi
    
    #
    # Adding an ACL that actually removes some access rights.
    #
    print_title "Adding Restrictive ACL Entry"
    mys9s tree --add-acl --acl="other::---" /home
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while adding ACL entry."
        exit 1
    fi

    mys9s tree --get-acl /home
    lines=$(s9s tree --get-acl /home)
    expected="^other::---$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi
}

#
# Removing the folder.
#
function testRmdir()
{
    local exitCode 
    local expected
    local lines

    print_title "Removing the Folder"

    # Creating a folder.
    mys9s tree --rmdir /home

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while removing a folder."
        exit 1
    fi

    return 0
}

#
# Running the requested tests.
#
startTests

reset_config

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCreateUsers
    runFunctionalTest checkTree01
    runFunctionalTest checkTree02
    runFunctionalTest testMkdir
    runFunctionalTest testAcl
    runFunctionalTest testRmdir
fi

endTests

