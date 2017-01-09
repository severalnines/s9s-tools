#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
MYDIR=$(readlink -m "$MYDIR")
VERSION="0.0.2"
VERBOSE=""
LOGFILE=""
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"

export LC_ALL=C

#
# Prints the software version and exits.
#
function printVersionAndExit()
{
    echo "$MYNAME Version $VERSION on $(hostname)" >&2
    exit 0
}

#
# $*: the error message
#
#
# Prints an error message to the standard error. The text will not mixed up with
# the data that is printed to the standard output.
#
function printError()
{
    local datestring=$(date "+%Y-%m-%d %H:%M:%S")

    echo -e "$MYNAME($$) $*" >&2

    if [ "$LOGFILE" ]; then
        echo -e "$datestring ERROR $MYNAME($$) $*" >>"$LOGFILE"
    fi
}

#
# $*: the message
#
# Prints all the arguments but only if the program is in the verbose mode.
#
function printVerbose()
{
    local datestring=$(date "+%Y-%m-%d %H:%M:%S")

    if [ "$VERBOSE" == "true" ]; then
        echo -e "$MYNAME($$) $*" >&2
    fi

    if [ "$LOGFILE" ]; then
        echo -e "$datestring DEBUG $MYNAME($$) $*" >>"$LOGFILE"
    fi
}

function printHelpAndExit()
{
cat <<EOF
Usage:
  $MYNAME [OPTION]... [TARGET_HOST]...

  $MYNAME - Monitors various system properties and reports them.

 -h, --help           Print this help and exit.
 -v, --version        Print version information and exit.
 --verbose            Print more messages.
 --log-file=FILE      Store all the messages in the given file too.

EXAMPLE

 ./install.sh core1 host01

EOF
    exit 0
}

ARGS=$(\
    getopt \
        -o hvs:c:l \
        -l "help,verbose,version,log-file:" \
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
            VERBOSE_OPTION="--verbose"
            ;;

        -v|--version)
            shift
            VERSION_OPTION="--version"
            ;;

        --log-file)
            shift
            LOGFILE=$(readlink -m "$1")
            shift
            ;;

        --)
            shift
            break
            ;;

        *)
            ;;
    esac
done

if [ -z "$*" ]; then
    printError "Missing command line arguments."
    exit 6
fi

for server in $*; do
    #$SSH $server -- [ ! -d bin ] && mkdir bin
    #scp pip-* "${server}:bin/"
    echo "Installing on host '$server'."
    $SSH $server -- mkdir install_sh_tmp
    scp pip-* "${server}:install_sh_tmp/"

    $SSH $server -- \
        sudo cp -vf install_sh_tmp/* /usr/bin && rm -rvf install_sh_tmp
done
