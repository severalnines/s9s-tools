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

function testSimpleJobs()
{
    local exit_code
    local n_lines

    #
    #
    #
    print_title "Testing Simple Jobs"

    # A job that destined to be a success.
    mys9s job --success --job-tags=success,test --wait
    exit_code=$?
    #echo "exit_code: $exit_code"
    if [ "$exit_code" -ne 0 ]; then
        failure "A success job should return 0."
    fi

    # And one that destined to be a failure.
    mys9s job --fail --job-tags=fail,test --wait
    exit_code=$?
    #echo "exit_code: $exit_code"
    if [ "$exit_code" -ne 1 ]; then
        failure "A fail job should return 1."
    fi
    
    #
    # Checking some output.
    #
    mys9s job --list --long 

    n_lines=$(s9s job --list --batch --show-failed | wc -l)
    if [ "$n_lines" -ne 1 ]; then
        failure "There should be one failed job."
    fi
    
    n_lines=$(s9s job --list --batch --show-finished | wc -l)
    if [ "$n_lines" -ne 1 ]; then
        failure "There should be one finished job ('$n_lines')."
    fi

    n_lines=$(s9s job --list --batch --job-tags=test | wc -l)
    if [ "$n_lines" -ne 2 ]; then
        failure "There should be 2 jobs with the tag 'test'."
    fi
    
    n_lines=$(s9s job --list --batch --job-tags=success | wc -l)
    if [ "$n_lines" -ne 1 ]; then
        failure "There should be 1 jobs with the tag 'success'."
    fi
    
    n_lines=$(s9s job --list --batch | wc -l)
    if [ "$n_lines" -ne 2 ]; then
        failure "There should be 2 jobs altogether."
    fi

    return 0
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
    runFunctionalTest testSimpleJobs 
fi

endTests

