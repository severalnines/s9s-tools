#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)

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
 --print-commands Do not print unit test info, print the executed commands.
 --reset-config   Remove and re-generate the ~/.s9s directory.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,print-commands,reset-config" \
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

if [ -z "$OPTION_RESET_CONFIG" ]; then
    printError "This script must remove the s9s config files."
    printError "Make a copy of ~/.s9s and pass the --reset-config option."
    exit 6
fi

reset_config

first=$(getent passwd $USER | cut -d ':' -f5 | cut -d ',' -f1 | cut -d ' ' -f1)
last=$(getent passwd $USER | cut -d ':' -f5 | cut -d ',' -f1 | cut -d ' ' -f2)  

#
# Here is how we can bootsrat the system by creating a user with superuser
# privileges and a password.
#
# The config directory ~/.s9s now removed, so no credentials there. The command
# line options define no --cmon-user either. When this happens, when the s9s
# command line tool can not find a cmon user to auhenticate it tries to send the
# request through a named pipe. Only "user --create" can be sent there, and this
# is exactly what we send here.
#
# To access the named pipe the s9s client has to be (1) run on the controller
# and have access to the named pipe through the local filesystem or (2) have ssh
# passwordless access to the controller where it finds the named pipe. The
# controller here is localhost, so the s9s in this example will send the request
# through /var/lib/cmon/usermgmt.fifo on the localhost.
#
# The command line here will create a new account with name "admin" and set the
# password to "admin". A public/private key pair will also be created and placed
# into ~/.s9s/admin.key and ~/.s9s/admin.pub. The public key will be posted to
# the controller to be used for authenticating the "admin" user later.
#
# The controller url, the "admin" username and the key file location will also
# be stored in the s9s configuration file ("~/.s9s/s9s.conf") so the next time
# the s9s program is run it can authenticate automatically. No need to pass any
# credentials.
#
# One more thing: the user created here will be placed into the admins group.
# The members of this group all have the superuser privileges, so this user will
# be able to do anything.
#
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
    "admin"


#
# Ok, now we have the username and the key file location stored in the s9s.conf
# file together with the controller URL. This means we can do anything without
# passing credentials through the command line:
#
mys9s user --list --long 

#
# But that does not mean we can't use other users or authenticate with a
# pasword. Here is how we can use the password:
#
mys9s user \
    --list \
    --long \
    --cmon-user="admin" \
    --password="admin"
