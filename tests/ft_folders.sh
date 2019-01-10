#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
MYDIR=$(readlink -m $MYDIR)

COMMAND_LINE_OPTIONS="$0 $*"

VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CONTAINER_SERVER=""
CONTAINER_IP=""

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

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

#
# This test will create a nested folder structure in one step. Then it will
# check if the folders are there and the hidden entries are indeed not shown
# without the --all command line option.
#
function testMkdir1()
{
    local lines
    local expected

    print_title "Creating Folders"
    cat <<EOF
  This test will create a nested folder structure in one step. Then it will
check if the folders are there and the hidden entries are indeed not shown
without the --all command line option.

EOF

    mys9s tree --mkdir /home/pipas/.config

    check_exit_code_no_job $?
    check_entry \
        --user         "$USER"      \
        --group        "testgroup"  \
        --acl          "drwxrwxrwx" \
        "/home"
    
    check_entry \
        --user         "$USER"      \
        --group        "testgroup"  \
        --acl          "drwxrwxrwx" \
        "/home/pipas"

    #mys9s tree --tree
    #mys9s tree --list

    lines=$(s9s tree --list)

    lines=$(s9s tree --list --recursive --full-path --all)
    expected="/home/pipas$"
    if ! echo "$lines" | grep --quiet "$expected"; then
        failure "Expected line not found: '$expected'"
        exit 1
    fi
    
    expected="./home/pipas/.config$"
    if echo "$lines" | grep --quiet "$expected"; then
        failure "Hidden entries should not be seen ('$expected')"
        exit 1
    fi

    mys9s tree --rmdir "/home/pipas/.config"
    check_exit_code_no_job $?
}

function testMkdir2()
{
    local folder_name="testMkdir2"
    local retcode

    print_title "Creating Folders with Failures"

    mys9s tree --mkdir "/$folder_name"
    check_exit_code_no_job $?
   
    check_entry \
        --user         "$USER"      \
        --group        "testgroup"  \
        --acl          "drwxrwxrwx" \
        "/$folder_name"
    
    # We should not be able to create a folder that already exists.
    mys9s tree --mkdir "/$folder_name"
    retcode=$?

    if [ $retcode -eq 0 ]; then
        failure "Creating the same folder should have failed."
    else
        success "  o Creating the same folder again failed, ok ($retcode)."
    fi

    mys9s tree --rmdir "/$folder_name"
    mys9s tree --tree --all

    return 0
}

function testTouch()
{
    local path="/home/pipas/test.text"
    local new_name="testfile.txt";
    local new_path="/home/pipas/testfile.txt"
    local invalid_path1="/.runtime/ak.txt"
    local invalid_path2="/no_such_folder/ak.txt"
    local retcode 

    print_title "Creating a File"

    mys9s tree --touch "$path"
    check_exit_code_no_job $?

    check_entry \
        --user         "$USER"      \
        --group        "testgroup"  \
        --acl          "-rwxrwxrwx" \
        --size         "0"          \
        "$path"

    mys9s tree --move "$path" "$new_name"
    check_exit_code_no_job $?
    
    check_entry \
        --user         "$USER"      \
        --group        "testgroup"  \
        --acl          "-rwxrwxrwx" \
        --size         "0"          \
        "$new_path"

    #
    # Trying to create a file where the user has no write access.
    #
    mys9s tree --touch "$invalid_path1"
    retcode="$?"

    if [ "$retcode" -eq 0 ]; then
        failure "Creating a file here should've failed."
    else
        success "  o Failed to create file with $retcode, ok."
    fi
   
    #
    # Trying to create a file in a folder that does not exist.
    #
    mys9s tree --touch "$invalid_path2"
    retcode="$?"

    if [ "$retcode" -eq 0 ]; then
        failure "Creating a file here should've failed."
    else
        success "  o Failed to create file with $retcode, ok."
    fi

    mys9s tree --tree --all
}

function testUser()
{
    print_title "Creating and Renaming a User"
    mys9s user \
        --create \
        --group="tos" \
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
        --user-name     "kirk" \
        --group         "tos" \
        --email-address "kirk@enterprise.com" \
        --password      "secret" \
        --check-key

    mys9s tree --mkdir "/home/kirk"
              
    mys9s tree \
        --chown \
        --owner=kirk:tos \
        --recursive \
        /home/kirk
    
    check_entry \
        --user         "kirk"       \
        --group        "tos"        \
        --acl          "drwxrwxrwx" \
        "/home/kirk"
   
    #
    #
    #
    print_title "Renaming a Group"
    
    mys9s tree --move /groups/tos TOS
    check_exit_code_no_job $?    
    
    check_entry \
        --user         "system"     \
        --group        "admins"     \
        --acl          "grwxrw----" \
        "/groups/TOS"

    check_entry \
        --user         "kirk"       \
        --group        "TOS"        \
        --acl          "drwxrwxrwx" \
        "/home/kirk"
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
else
    runFunctionalTest testMkdir1
    runFunctionalTest testMkdir2
    runFunctionalTest testTouch
    runFunctionalTest testUser
fi

endTests
