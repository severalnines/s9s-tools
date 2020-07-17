#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

cd $MYDIR
source ./include.sh

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

function testCreateUsers()
{
    print_title "Creating a User"

    begin_verbatim
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
    else
        success "  o The exit code is 0, OK."
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
        --new-password="supervisor" \
        "supervisor"

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating user through RPC"
    else
        success "  o The exit code is 0, OK."
    fi

    end_verbatim
}

function checkTree01()
{
    local expected
    local lines

    print_title "Checking Tree List"

    begin_verbatim
    mys9s tree --cmon-user=supervisor --list --long --directory /

    # The root directory owned by the system user.
    lines=$(s9s tree --cmon-user=supervisor --list --long --directory /)
    mys9s tree --cmon-user=supervisor --list --long --directory /

    expected="^drwxrwxrwx     - system admins /$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
    else
        success "  o Expected line '$expected' found, OK."
    fi

    # The admin user has a link that the admin will see
    lines=$(s9s tree --cmon-user=supervisor --list --long groups/admins)
    mys9s tree --cmon-user=supervisor --list --long groups/admins

    # Symlinks are not created any more.
#    expected="^urwxr--r--     - supervisor admins supervisor -> /supervisor$"
#    if ! find_line "$lines" "$expected"; then
#        failure "Expected line not found: '$expected'"
#    else
#        success "  o Expected line '$expected' found, OK."
#    fi

    # Symlinks are not created any more.
    # The non-admin user has a link too.
#    lines=$(s9s tree --cmon-user=supervisor --list --long groups/users)
#    mys9s tree --cmon-user=supervisor --list --long groups/users
#
#    expected="^urwxr--r--     - pipas  admins pipas -> /pipas$"
#    if ! find_line "$lines" "$expected"; then
#        failure "Expected line not found: '$expected'"
#    else
#        success "  o Expected line '$expected' found, OK."
#    fi
    
    # The user owns itself.
    lines=$(s9s tree --cmon-user=supervisor --list --long)
    mys9s tree --cmon-user=supervisor --list --long

    expected="^urwxr--r--     - pipas  admins pipas$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
    else
        success "  o Expected line '$expected' found, OK."
    fi

    end_verbatim
}

function checkTree02()
{
    print_title "Checking Access Rights"

    begin_verbatim
    mys9s tree --access --privileges="rwx" --cmon-user="supervisor" /
    if [ $? -ne 0 ]; then
        failure "User 'supervisor' ha no access to '/'."
    else 
        success "  o User 'supervisor' ha access to '/', OK."
    fi

    mys9s tree --access --privileges="rwx" --cmon-user="pipas" /
    if [ $? -ne 0 ]; then
        failure "User 'pipas' ha no access to '/'"
    else
        success "  o User 'pipas' ha access to '/', OK."
    fi

    end_verbatim
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

    begin_verbatim
    # Creating a folder.
    mys9s tree --mkdir /home

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while creating a folder"
    else
        success "  o The exit code is 0, OK."
    fi

    # Checking the new folder.
    mys9s tree --cmon-user=supervisor --list --long
    lines=$(s9s tree --cmon-user=supervisor --list --long)
    
    expected="^drwxrwxrwx     - pipas  users  home$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
    else
        success "  o Expected line '$expected' found, OK."
    fi

    end_verbatim
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

    begin_verbatim
    mys9s tree --get-acl /home
    
    lines=$(s9s tree --get-acl /home)
    expected="^user::rwx$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s tree --get-acl /home
    else
        success "  o Expected line '$expected' found, OK."
    fi
    
    expected="^group::rwx$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s tree --get-acl /home
    else
        success "  o Expected line '$expected' found, OK."
    fi
    
    expected="^other::rwx$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
        mys9s tree --get-acl /home
    else
        success "  o Expected line '$expected' found, OK."
    fi
    
    end_verbatim
}

function testAddAcl()
{
    # 
    # We are now adding a new ACL to the folder, a real ACL entry with a user.
    #
    print_title "Adding ACL Entry"
    begin_verbatim

    mys9s tree --add-acl --acl="user:nobody:r-x" /home
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while adding ACL entry."
    else
        success "  o The exit code is 0, OK."
    fi
    
    mys9s tree --get-acl /home
    lines=$(s9s tree --get-acl /home)
    expected="^user:nobody:r-x$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
    else
        success "  o Expected line '$expected' found, OK."
    fi
    
    end_verbatim
}

function testRetrictiveAcl()
{
    #
    # Adding an ACL that actually removes some access rights.
    #
    print_title "Adding Restrictive ACL Entry"
    begin_verbatim
    
    mys9s tree --add-acl --acl="other::---" /home
    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while adding ACL entry."
    else
        success "  o The exit code is 0, OK."
    fi

    mys9s tree --get-acl /home
    lines=$(s9s tree --get-acl /home)
    expected="^other::---$"
    if ! find_line "$lines" "$expected"; then
        failure "Expected line not found: '$expected'"
    else
        success "  o Expected line '$expected' found, OK."
    fi

    end_verbatim
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

    begin_verbatim
    mys9s tree --rmdir /home

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode} while removing a folder."
    else
        success "  o The exit code is 0, OK."
    fi

    end_verbatim
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
    runFunctionalTest testAddAcl
    runFunctionalTest testRetrictiveAcl
    runFunctionalTest testRmdir
fi

endTests

