#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
SERVER="core1"

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
            SERVER="$1"
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

reset_config

first=$(getent passwd $USER | cut -d ':' -f5 | cut -d ',' -f1 | cut -d ' ' -f1)
last=$(getent passwd $USER | cut -d ':' -f5 | cut -d ',' -f1 | cut -d ' ' -f2)  

print_title "Test is under construction"

mys9s user \
    --create \
    --generate-key \
    --group="admins" \
    --new-password="admin" \
    --controller="https://localhost:9556" \
    --email-address="laszlo@severalnines.com" \
    --first-name="$first" \
    --last-name="$last" \
    $OPTION_PRINT_JSON \
    $OPTION_VERBOSE \
    "pipas"

mys9s user --list --long 

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode}"
    exit 1
fi

mys9s server \
    --register \
    --servers="lxc://core1?;lxc://storage01"

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode}"
    exit 1
fi

mys9s server \
    --tree

exitCode=$?
if [ "$exitCode" -ne 0 ]; then
    failure "The exit code is ${exitCode}"
    exit 1
fi

