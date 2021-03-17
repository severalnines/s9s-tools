
S9S=$(which s9s)
FAILED="no"
TEST_SUITE_NAME=""
TEST_NAME=""
DONT_PRINT_TEST_MESSAGES=""
PRINT_COMMANDS=""
PRINT_PIP_COMMANDS=""
OPTION_KEEP_NODES=""
TEST_EMAIL="laszlo@severalnines.com"

# 
# If this variable is exported all the requests will be printed for debugging
# purposes.
#
#export S9S_DEBUG_PRINT_REQUEST="true"

#
# If this variable is exported one request and the reply will be saved of every
# type into this directory. This is not really for debugging, it is mostly for
# creating a documentation (not all the requests/replies are saved, only one of
# every request types).
#
#export S9S_DEBUG_SAVE_REQUEST_EXAMPLES="request-examples"

if [ "${S9S_TEST_EMAIL}" != "" ]; then
	export TEST_EMAIL=${S9S_TEST_EMAIL}
fi
if [ "${S9S_TEST_CONTAINER_SERVER}" != "" ]; then
	export CONTAINER_SERVER=${S9S_TEST_CONTAINER_SERVER}
fi
if [ "${S9S_TEST_PRINT_COMMANDS}" != "" ]; then
	export PRINT_COMMANDS=${S9S_TEST_PRINT_COMMANDS}
fi

export SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet"

#
# Convenience variables to use the IP addresses of the created nodes in the
# tests.
#
export FIRST_ADDED_NODE=""
export SECOND_ADDED_NODE=""
export THIRD_ADDED_NODE=""
export FOURTH_ADDED_NODE=""
export FIFTH_ADDED_NODE=""
export LAST_ADDED_NODE=""

NUMBER_OF_SUCCESS_CHECKS=0
NUMBER_OF_WARNING_CHECKS=0
NUMBER_OF_FAILED_CHECKS=0
NUMBER_OF_PERFORMED_CHECKS=0

TERM_NORMAL="\033[0;39m"
TERM_BOLD="\033[1m"
XTERM_COLOR_RED="\033[0;31m"
XTERM_COLOR_GREEN="\033[0;32m"
XTERM_COLOR_ORANGE="\033[0;33m"
XTERM_COLOR_BLUE="\033[0;34m"
XTERM_COLOR_PURPLE="\033[0;35m"
XTERM_COLOR_CYAN="\033[0;36m"
XTERM_COLOR_LIGHT_GRAY="\033[0;37m"
XTERM_COLOR_DARK_GRAY="\033[1;30m"
XTERM_COLOR_LIGHT_RED="\033[1;31m"
XTERM_COLOR_LIGHT_GREEN="\033[1;32m"
XTERM_COLOR_YELLOW="\033[1;33m"
XTERM_COLOR_LIGHT_BLUE="\033[1;34m"
XTERM_COLOR_LIGHT_PURPLE="\033[1;35m"
XTERM_COLOR_LIGHT_CYAN="\033[1;36m"
XTERM_COLOR_WHITE="\033[1;37m"

TERM_COLOR_TITLE="\033[1m\033[37m"
CMON_CONTAINER_NAMES=""

#export S9S_DEBUG_PRINT_REQUEST="true"

if [ -x ../s9s/s9s ]; then
    S9S="../s9s/s9s"
fi

#
# Prints the arguments but only if the output does not go to the standard
# output.
#
function print_html()
{
    if [ ! -t 1 ]; then
        echo $*
    fi
}

function color_command()
{
    sed \
        -e "s#\(--.\+\)=\([^\\]*\)[\ ]#\x1b\[0;33m\1\x1b\[0;39m=\"\x1b\[1;35m\2\x1b\[0;39m\" #g" \
        -e "s#\(--[^\\\ ]\+\)#\x1b\[0;33m\1\x1b\[0;39m#g"
}

function prompt_string
{
    local dirname=$(basename $PWD)
    local DATETIME=""

    if [ "${S9S_TEST_DATETIME_PROMPT}" != "" ]; then
        DATETIME="$(date +"%Y-%m-%dT%X") "
    fi

    echo "$DATETIME$USER@$HOSTNAME:$dirname\$"
}

#
# Prints the sleep message and also executes it.
#
# EXAMPLE:
#  mysleep 10
#
function mysleep()
{
    local prompt=$(prompt_string)
        
    echo -ne "$prompt ${XTERM_COLOR_YELLOW}sleep${TERM_NORMAL} "
    echo -ne "${XTERM_COLOR_ORANGE}$@${TERM_NORMAL}"
    echo ""
    sleep $@
}

function my_command()
{
    local hostname=$(hostname)
    local dirname=$(basename $PWD)
    local username="$USER"
    local prompt
    local the_command
    local argument
    
    while [ -n "$1" ]; do
        case "$1" in
            --hostname)
                hostname="$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    prompt="$XTERM_COLOR_GREEN$username@$hostname:$dirname\$$TERM_NORMAL"

    the_command="$1"
    shift

    #
    # Printing the command.
    #
    echo -ne "$prompt ${XTERM_COLOR_YELLOW}${the_command}${TERM_NORMAL} "
    while [ "$the_command" == "nohup" -o "$the_command" == "sudo" ]; do
        the_command="$1"
        echo -ne "${XTERM_COLOR_YELLOW}${the_command}${TERM_NORMAL} "
        shift
    done

    for argument in "$@"; do
        case "$argument" in
            -*)
                echo -ne " ${XTERM_COLOR_ORANGE}$argument${TERM_NORMAL}"
                ;;
            *)
                echo -ne " ${XTERM_COLOR_BLUE}$argument${TERM_NORMAL}"
                ;;
        esac
    done

    echo ""
}

function my_exec()
{
    my_command $@
    $@
}

function mys9s_singleline()
{
    local prompt=$(prompt_string)
    local nth=0
    local retcode

    if [ -f "/tmp/LAST_COMMAND_OUTPUT" ]; then
        rm -f "/tmp/LAST_COMMAND_OUTPUT"
    fi 

    if [ "$PRINT_COMMANDS" ]; then
        echo -ne "$prompt ${XTERM_COLOR_YELLOW}s9s${TERM_NORMAL} "

        for argument in "$@"; do
            #if [ $nth -gt 0 ]; then
            #    echo -e "\\"
            #fi

            if [ $nth -eq 0 ]; then
                echo -ne "${XTERM_COLOR_BLUE}$argument${TERM_NORMAL}"
            elif [ $nth -eq 1 ]; then
                echo -ne " ${XTERM_COLOR_ORANGE}$argument${TERM_NORMAL}"
            else
                echo -ne " $argument" | color_command
            fi

            let nth+=1
        done
    
        echo ""
    fi

    $S9S --color=always "$@" 2>&1 | tee /tmp/LAST_COMMAND_OUTPUT
    retcode=${PIPESTATUS[0]}
    
    return $retcode
}

function mys9s_multiline()
{
    local prompt=$(prompt_string)
    local nth=0
    local retcode
    
    if [ -f "/tmp/LAST_COMMAND_OUTPUT" ]; then
        rm -f "/tmp/LAST_COMMAND_OUTPUT"
    fi 

    if [ "$PRINT_COMMANDS" ]; then
        echo ""
        echo -ne "$prompt ${XTERM_COLOR_YELLOW}s9s${TERM_NORMAL} "

        for argument in "$@"; do
            if [ $nth -gt 0 ]; then
                echo -e "\\"
            fi

            if [ $nth -eq 0 ]; then
                echo -ne "${XTERM_COLOR_BLUE}$argument${TERM_NORMAL} "
            elif [ $nth -eq 1 ]; then
                echo -ne "    ${XTERM_COLOR_ORANGE}$argument${TERM_NORMAL} "
            else
                echo -ne "    $argument " | color_command
            fi

            let nth+=1
        done
    
        echo ""
        echo ""
    fi

    $S9S --color=always "$@" 2>&1 | tee /tmp/LAST_COMMAND_OUTPUT
    retcode=${PIPESTATUS[0]}

    return $retcode
}

function S9S_LAST_OUTPUT_CONTAINS()
{
    local retval=0

    if [ ! -f "/tmp/LAST_COMMAND_OUTPUT" ]; then
        failure "File '/tmp/LAST_COMMAND_OUTPUT' was not found."
        return 1
    fi

    while [ -n "$1" ]; do
        if cat "/tmp/LAST_COMMAND_OUTPUT" | grep -q "$1"; then
            success "  o Text '$1' found in the output, OK."
        else
            failure "Line '$1' was not found in the output."
            retval=1
        fi

        shift
    done

    return $retval
}

#
# Prints the s9s command line and also executes it.
#   $@   The command line options and arguments for the s9s program.
#
function mys9s()
{
    local n_arguments=0
    local argument

    for argument in $*; do
        let n_arguments+=1
    done

    if [ "$n_arguments" -lt 4 ]; then
        mys9s_singleline "$@"
    else
        mys9s_multiline "$@"
    fi
}

function print_command()
{
    local n_arguments=0
    local argument
    local prompt=$(prompt_string)
    local nth=0
    local cmd="$1"
    shift

    for argument in $*; do
        let n_arguments+=1
    done

    echo ""
    echo -ne "$prompt ${XTERM_COLOR_YELLOW}${cmd}${TERM_NORMAL} "

    for argument in "$@"; do
        if [ 4 -lt "$n_arguments" ]; then
            if [ $nth -gt 0 ]; then
                echo -e "\\"
            fi
        fi

        if [ $nth -eq 0 ]; then
            echo -ne "${XTERM_COLOR_BLUE}$argument${TERM_NORMAL} "
        elif [ $nth -eq 1 ]; then
            echo -ne "    ${XTERM_COLOR_ORANGE}$argument${TERM_NORMAL} "
        else
            echo -ne "    $argument " | color_command
        fi

        let nth+=1
    done

    echo ""
    echo ""
}

t1_counter="0"
t2_counter="0"

#
# Prints a title line that looks nice on the terminal and also on the web.
#
function print_title()
{
    local number

    #let t1_counter+=1
    #let t2_counter=0

    number="${t1_counter}"
    if [ -t 1 ]; then
        echo ""
        echo -e "$TERM_COLOR_TITLE$number $*\033[0;39m"
        echo -e "\033[1m\
-------------------------------------------------------------------------------\
-\033[0;39m"
    else
        printf "<h3>%03d $*</h3>\n" "$number"
    fi
}

#
# Same as print_title(), but this is for second level titles.
#
function print_subtitle()
{
    local number
    let t2_counter+=1

    number="${t1_counter}.${t2_counter}"
    if [ -t 1 ]; then
        echo ""
        echo -e "$TERM_COLOR_TITLE$number $*\033[0;39m"
    else
        echo ""
        echo ""
        echo ""
        echo "<h4>$number $*</h4>"
    fi
}

function begin_verbatim()
{
    if [ ! -t 1 ]; then
        echo "<pre>"
    fi
}

function end_verbatim()
{
    if [ ! -t 1 ]; then
        echo "</pre>"
    fi
}

function paragraph()
{
    [ ! -t 1 ] && echo -n "<p>"
    cat | sed -e 's/^  //g'
    [ ! -t 1 ] && echo -ne "</p>\n\n" 
}

#
# This function should be used as a filter to colorize configuration files.
#
function print_ini_file()
{
     if [ -t 1 ]; then
         highlight --syntax=ini --out-format=xterm256
     else
         highlight --syntax=ini \
             --out-format=html \
             --inline-css \
             --fragment \
             --enclose-pre
     fi
}

#
# This function should be called before the functional tests are executed.
# Currently this only prints a message for the user, but this might change.
#
function startTests ()
{
    local container_list_file="/tmp/${MYNAME}.containers"
    local model
    local memory

    TEST_SUITE_NAME=$(basename $0 .sh)

    #
    # Printing some info
    #
    print_title "Preparing to Run Test Script"
    cat <<EOF
  The startTests BASH function is preparing to execute the test script.
EOF

    begin_verbatim
    pip-host-control --status="Running '$TEST_SUITE_NAME'."

    echo "Starting test $TEST_SUITE_NAME"
    if [ -n "$COMMAND_LINE_OPTIONS" ]; then
        echo "  Command line: $COMMAND_LINE_OPTIONS"
    fi

    model=$(cat /sys/devices/virtual/dmi/id/product_name | tr -d '\n')
    memory=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    let memory/=1024
    let memory/=1024

    cat <<EOF
     TEST_SUITE_NAME: $TEST_SUITE_NAME

            hostname: $(hostname)
               model: $model
             threads: $(nproc)
    memory_gigabytes: $memory

               MYDIR: $MYDIR
             VERSION: $VERSION
                USER: $USER
                 PWD: $PWD

EOF

    #
    # Doing some checks
    #
    if [ -z "$(which jq)" ]; then
        sudo apt-get install -y --force-yes jq
    fi
   
    CHECK_PROGRAM_INSTALLED jq
    CHECK_PROGRAM_INSTALLED pip-container-create
    CHECK_PROGRAM_INSTALLED highlight

    if [ -z "$S9S" ]; then
        failure "The 's9s' program is not installed."
    else 
        success "  o The 's9s' program is installed, ok."
    fi

    #
    # Cleanups before the test.
    #
    if [ -f "$container_list_file" ]; then
        echo "Removing '$container_list_file'."
        rm -f "$container_list_file"
    fi
    
    if [ -f "$HOME/.s9s/s9s.state" ]; then
        echo "Removing '$HOME/.s9s/s9s.state'."
        rm -f $HOME/.s9s/s9s.state
    fi

    rm -rf /tmp/BACKUP*

    if [ -d "$MYDIR/../pipscripts" ]; then
        echo "Installing the 'pipscripts' script set."
        echo "Entering directory pipscripts..."
        pushd "$MYDIR/../pipscripts" >/dev/null
        sudo make install 
        popd >/dev/null
    fi

    if [ -f "$HOME/.s9s/s9s.state" ]; then
        echo "Removing '$HOME/.s9s/s9s.state'."
        rm -f $HOME/.s9s/s9s.state
    fi

    if [ -f "/etc/cmon-ldap.cnf" ]; then
        echo "Removing '/etc/cmon-ldap.cnf'..."
        sudo mv "/etc/cmon-ldap.cnf" "/etc/cmon-ldap.cnf.BAK"
    fi

    if [ -f "$HOME/.pip/host01.host" ]; then
        pip-server-control --list --long --print-report $(hostname) core4 storage02
    fi

    end_verbatim
}

function check_reply()
{
    local lines
    local name
    local required
    local value

    lines="$1"
    shift

    while [ -n "$1" ]; do
        name="$1"
        required="$2"

        value=$(echo "$lines" | jq "$name" | tr -d '"')

        if [ "$value" == "$required" ]; then
            success "  o Value for $name is $value, OK."
        else
            failure "Value for $name is $value, should be $required."
        fi

        shift 2
    done
}

#
# This function should be called when the function tests are executed. It prints
# a message about the results and exits with the exit code that is true if the
# tests are passed and false if at least one test is failed.
#
function endTests ()
{
    local exit_code=0

    if isSuccess; then
        if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
            echo "SUCCESS: $(basename $0 .sh)"
        else
            print_title "Report"
            begin_verbatim

            echo -en "${XTERM_COLOR_GREEN}"
            echo -n  "Test $(basename $0) is successful."
            echo -en "${TERM_NORMAL}"
            echo ""
        fi

        pip-host-control --status="Passed '$TEST_SUITE_NAME'."
        
        exit_code=0
    else
        if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
            echo "FAILURE: $(basename $0 .sh)"
        else
            print_title "Report"
            begin_verbatim
            echo -en "${XTERM_COLOR_RED}"
            echo -n  "Test $(basename $0) has failed."
            echo -en "${TERM_NORMAL}"
            echo ""
        fi
    
        pip-host-control --status="Failed '$TEST_SUITE_NAME'."
        exit_code=1
    fi

    printf "  Performed: %'4d check(s)\n" "$NUMBER_OF_PERFORMED_CHECKS"
    printf "    Success: %'4d check(s)\n" "$NUMBER_OF_SUCCESS_CHECKS"
    printf "    Warning: %'4d check(s)\n" "$NUMBER_OF_WARNING_CHECKS"
    printf "     Failed: %'4d check(s)\n" "$NUMBER_OF_FAILED_CHECKS"
    end_verbatim

    exit $exit_code
}

#
# Returns true if none of the tests failed before, false if some bug was
# detected.
#
function isSuccess 
{
    if [ "$FAILED" == "no" ]; then
        return 0
    fi

    return 1
}

#
# Returns true if the --verbose option was provided.
#
function isVerbose 
{
    if [ "$VERBOSE" == "true" ]; then
        return 0
    fi

    return 1
}

#
# Prints the message passed as command line options if the test is executet in
# verbose mode (the --verbose command line option was provided).
#
function printVerbose 
{
    isVerbose && echo "$@" >&2
}

#
# Prints a message about the failure and sets the test failed state.
#
function failure
{
    if [ "$TEST_SUITE_NAME" -a "$TEST_NAME" ]; then
        echo -en "$TEST_SUITE_NAME::$TEST_NAME(): "
        echo -e  "${XTERM_COLOR_RED}$1${TERM_NORMAL}"
    else
        echo -e  "${XTERM_COLOR_RED}$1${TERM_NORMAL}"
    fi

    echo "p42err $1"
    let NUMBER_OF_PERFORMED_CHECKS+=1
    let NUMBER_OF_FAILED_CHECKS+=1
    FAILED="true"
}

#
# Prints a message about the failure and sets the test failed state.
#
function success()
{
    let NUMBER_OF_SUCCESS_CHECKS+=1
    let NUMBER_OF_PERFORMED_CHECKS+=1

    echo -e "${XTERM_COLOR_GREEN}$1${TERM_NORMAL}"
}

#
# Prints a warning message and registers that an other test caused a warning.
#
function warning()
{
    let NUMBER_OF_WARNING_CHECKS+=1
    let NUMBER_OF_PERFORMED_CHECKS+=1

    echo -e "${XTERM_COLOR_YELLOW}$1${TERM_NORMAL}"
}

function CHECK_PROGRAM_INSTALLED()
{
    local program="$1"
    local path=$(which $program)

    if [ -z "$path" ]; then
        failure "The '$program' is not installed."
    else
        success "  o The '$program' utilitity is installed, ok."
    fi
}

#
# This will check the exit code passed as an argument and print the logs
# of the last failed job if the exit code is not 0.
#
function check_exit_code()
{
    local do_not_exit
    local exitCode
    local jobId
    local password_option=""

    if [ -n "$CMON_USER_PASSWORD" ]; then
        password_option="--password='$CMON_USER_PASSWORD'"
    fi

    #
    # Command line options.
    #
    while true; do
        case "$1" in 
            --do-not-exit)
                shift
                do_not_exit="true"
                ;;

            *)
                break
                ;;
        esac
    done

    exitCode="$1"

    #
    # Checking...
    #
    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}"

        jobId=$(\
            s9s job --list --batch $password_option | \
            grep FAIL | \
            tail -n1 | \
            awk '{print $1}')

        if [ "$jobId" -a "$LOG_OPTION" != "--log" ]; then
            echo "A job is failed. The test script will now try to list the"
            echo "job messages of the failed job. Here it is:"
            mys9s job \
                --log \
                $password_option \
                --debug \
                --job-id="$jobId"
        fi

        # Because of Jenkins we can not exit!
#        if [ "$do_not_exit" ]; then
            return 1
#        else
#            end_verbatim
#            exit $exitCode
#        fi
    else
        success "  o The exit code is $exitCode, ok"
    fi

    return 0
}

function check_exit_code_no_job()
{
    local exitCode="$1"

    if [ "$exitCode" -ne 0 ]; then
        failure "The exit code is ${exitCode}."
        return 1
    else
        success "  o The exit code is 0, ok."
    fi

    return 0
}

#
# This function is for checking a job, most importantly its state.
#
function check_job()
{
    local job_id
    local state
    local required_state
    local do_warning

    while true; do
        case "$1" in 
            --job-id)
                shift
                job_id="$1"
                shift
                ;;

            --state)
                shift
                required_state="$1"
                shift
                ;;

            --warning)
                shift
                do_warning="true"
                ;;

            *)
                break
                ;;
        esac
    done

    state=$(s9s job --list --job-id=$job_id --batch | awk '{print $3}')
    if [ "$state" == "$required_state" ]; then
        success "  o Job $job_id is $state, ok"
    else
        if [ -n "$do_warning" ]; then
            warning "Job $job_id state is $state, not $required_state."
        else
            failure "Job $job_id state is $state, not $required_state."
        fi
    fi
}

#
# This function is for checking if certain messages are n the cmon log (not the
# job messages, the log).
#
function check_log_messages()
{
    local log_file="/tmp/cmon.log"
    local line
    local cmon_user="system"
    local password="secret"

    while true; do
        case "$1" in 
            --cmon-user)
                cmon_user="$2"
                shift 2
                ;;

            --password)
                password="$2"
                shift 2
                ;;

            --)
                break
                ;;

            *)
                break
                ;;
        esac
    done
    
    mys9s log \
        --cmon-user="$cmon_user" \
        --password="$password" \
        --list \
        --long \
        --log-format="%I %M\n" \
        --cluster-id=0

    if [ $? -ne 0 ]; then
        failure "Failed to get the log messages."
    fi

    for line in "$@"; do

        s9s log \
            --cmon-user="$cmon_user" \
            --password="$password" \
            --list \
            --long \
            --log-format="%I %M\n" \
            --cluster-id=0 | \
        grep --quiet "$line"

        if [ $? -eq 0 ]; then
            success "  o Found in log: '$line'"
        else
            failure "Not found in log: '$line'"
        fi
    done
    
    if [ -n "$log_file" ]; then
        success "  o Will check log file '$log_file', ok."

        if [ -f "$log_file" ]; then
            success "  o Log file '$log_file' exists, ok."
        else
            failure "Log file '$log_file' does not exist."
        fi

        for line in "$@"; do
            if grep --quiet "$line" $log_file; then
                success "  o Found in file: '$line'"
            else
                failure "Not found in file: '$line'"
            fi
        done
    fi
}

#
# This debug function is for printing the cmon log (not the job messages, the
# log).
#
function print_log_messages()
{
    print_subtitle "Cmon Logs"

    s9s log \
        --list \
        --long \
        --log-format="%02i %04I %18c %38B:%5L %-8S %M\n" \
        --cluster-id=0 \
        --limit=100 \
        --color="always" \
        --debug 

    print_subtitle "Cmon Logs with Templates"
    s9s log --list --long --log-format-file='templates/log/%c_${log_specifics/job_instance/job_spec/command};templates/log/%c'
}

function print_log_message()
{
    local message_id="$1"

    s9s log --list \
        --log-format='   ID: %I\nclass: %c\n  loc: %B:%L\n mess: %M\n job:\n${/log_specifics/job_instance}\n' \
        --message-id=$message_id
}

function get_log_message_id()
{
    local job_command
    local lines
    local log_format
    local job_class
    local cluster_id=0

    while true; do
        case "$1" in 
            --job-command)
                job_command="$2"
                shift 2
                ;;

            --job-class)
                job_class="$2"
                shift 2
                ;;

            --)
                break
                ;;

            *)
                break
                ;;
        esac
    done

    log_format+='%I '
    log_format+='%c '
    log_format+='${/log_specifics/job_instance/job_spec/command} '
    log_format+='\n'

    lines=$(s9s log \
        --list \
        --batch \
        --log-format="$log_format" \
        --cluster-id="$cluster_id" \
        --cmon-user=system \
        --password=secret)

    if [ -n "$job_command" ]; then
        lines=$(echo "$lines" | awk "\$3 == \"$job_command\" { print \$0 }")
    fi
    
    if [ -n "$job_class" ]; then
        lines=$(echo "$lines" | awk "\$2 == \"$job_class\" { print \$0 }")
    fi

    echo $lines | tail -n 1 | awk '{ print $1 }'
}

function check_log_message()
{
    local message_id
    local format_string
    local expected_value
    local value
    local cluster_id=0
    
    while true; do
        case "$1" in 
            --message-id)
                message_id="$2"
                shift 2
                ;;

            --)
                break
                ;;

            *)
                break
                ;;
        esac
    done

    if [ -n "$message_id" ]; then
        success "  o Will check log message with ID $message_id, ok."
    else
        failure "Message ID unknown."
        return 1
    fi

    while true; do
        format_string="$1"
        expected_value="$2"

        if [ -z "$format_string" ]; then
            break
        fi

        mys9s log \
            --list --long \
            --log-format="$format_string" \
            --message-id="$message_id" \
            --cmon-user="system" \
            --password="secret" \
            --cluster-id="$cluster_id"

        value=$(s9s log \
                --list \
                --long \
                --batch \
                --log-format="$format_string" \
                --message-id="$message_id" \
                --cmon-user="system" \
                --password="secret" \
                --cluster-id="$cluster_id")

        if [ "$value" == "$expected_value" ]; then
            success "  o Value for '$format_string' is '$value', ok."
        else
            failure "Value for '$format_string' is '$value', not '$expected_value'."
        fi

        shift 2
    done

}

function check_container()
{
    local container_name
    local container_ip
    local owner
    local expected_owner="$USER"
    local group
    local expected_group
    local cloud
    local expected_cloud
    local state
    local expected_state
    local parent 
    local expected_parent
    local prot
    local expected_prot
    local path
    local expected_path
    local acl
    local expected_acl
    
    while [ -n "$1" ]; do
        case "$1" in
            --owner)
                expected_owner="$2"
                shift 2
                ;;

            --group)
                expected_group="$2"
                shift 2
                ;;
                
            --cloud)
                expected_cloud="$2"
                shift 2
                ;;

            --state)
                expected_state="$2"
                shift 2
                ;;

            --parent|--server)
                expected_parent="$2"
                shift 2
                ;;

            --prot)
                expected_prot="$2"
                shift 2
                ;;

            --path)
                expected_path="$2"
                shift 2
                ;;

            --acl)
                expected_acl="$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    container_name="$1"

    #
    # Checking the container IP.
    #
    container_ip=$(\
        s9s server \
            --list-containers \
            --batch \
            --long  "$container_name" \
        | awk '{print $6}')
    
    if [ -z "$container_ip" -o "$container_ip" == "-" ]; then
        failure "The container was not created or got no IP."
        s9s container --list --long
    else
        success "  o Container $container_name has IP $container_ip, ok"
    fi
 
    owner=$(\
        s9s container --list --long --batch "$container_name" | \
        awk '{print $4}')

    if [ "$owner" != "$expected_owner" ]; then
        failure "The owner is '$owner', should be '$expected_owner'"
    else
        success "  o The owner of the container is '$owner', ok"
    fi

    if ! is_server_running_ssh "$container_ip" "$owner"; then
        failure "User $owner can not log in to $container_ip"
    else
        success "  o SSH access granted for user '$USER' on $container_ip, ok."
    fi

    group=$(s9s container --list --batch --container-format="%G\n" "$container_name")
    if [ -n "$expected_group" -a "$group" != "$expected_group" ]; then
        failure "  o The group should be '$expected_group', not '$group'."
    else
        success "  o The group of the container is $group, ok"
    fi

    path=$(s9s container --list --batch --container-format="%p\n" "$container_name")
    if [ -n "$expected_path" -a "$path" != "$expected_path" ]; then
        failure "  o The path should be '$expected_path', not '$path'."
    else
        success "  o The path of the container is $path, ok"
    fi
    
    acl=$(s9s container --list --batch --container-format="%l\n" "$container_name")
    if [ -n "$expected_acl" -a "$acl" != "$expected_acl" ]; then
        failure "  o The acl should be '$expected_acl', not '$acl'."
    else
        success "  o The acl of the container is '$acl', ok"
    fi

    cloud=$(s9s container --list --batch --container-format="%c\n" "$container_name")
    if [ -n "$expected_cloud" -a "$cloud" != "$expected_cloud" ]; then
        failure "  o The cloud should be '$expected_cloud', not '$cloud'."
    else
        success "  o The cloud of the container is $cloud, ok"
    fi

    state=$(s9s container --list --batch --container-format="%S\n" "$container_name")
    if [ -n "$expected_state" -a "$state" != "$expected_state" ]; then
        failure "  o The state should be '$expected_state', not '$state'."
    else
        success "  o The state of the container is $state, ok"
    fi

    parent=$(s9s container --list --batch --container-format="%P\n" "$container_name")
    if [ -n "$expected_parent" -a "$parent" != "$expected_parent" ]; then
        failure "  o The parent should be '$expected_parent', not '$parent'."
    else
        success "  o The parent of the container is $parent, ok"
    fi

    prot=$(s9s container --list --batch --container-format="%T\n" "$container_name")
    if [ -n "$expected_prot" -a "$prot" != "$expected_prot" ]; then
        failure "  o The prot should be '$expected_prot', not '$prot'."
    else
        success "  o The prot of the container is $prot, ok"
    fi
}

#
# This function will check if a core file is created and fails the test if so.
#
function checkCoreFile 
{
    for corefile in data/core /tmp/cores/*; do 
        if [ -e "$corefile" ]; then
            failure "Some core file(s) found."
            ls -lha $corefile
        fi
    done
}

function find_line()
{
    local text="$1"
    local line="$2"
    local tmp

    while true; do
        tmp=$(echo "$text" | sed -e 's/  / /g')
        if [ "$tmp" == "$text" ]; then
            break
        fi

        text="$tmp"
    done

    while true; do
        tmp=$(echo "$line" | sed -e 's/  / /g')
        if [ "$tmp" == "$line" ]; then
            break
        fi

        line="$tmp"
    done

    printVerbose "text: $text"
    printVerbose "line: $line"

    echo "$text" | grep --quiet "$line"
    return $?
}

#
# Formatting the timestamp to a string that the cmnd will understand, e.g. 
# "2020-11-28T08:39:34.753+01:00".
#
function dateTimeToTZ()
{
    date +'%FT%T.%3N%:z' --date "$1" 2>/dev/null
}

#
# $1: the file in which the output messages are stored.
# $2: the message to find.
#
function checkMessage()
{
    local file="$1"
    local message="$2"

    if [ -z "$file" ]; then
        failure "Internal error: checkMessage() has no file name"
        return 1
    fi

    if [ -z "$message" ]; then
        failure "Internal error: checkMessage() has no message"
        return 1
    fi

    if grep -q "$message" "$file"; then
        return 0
    fi

    failure "Text '$message' missing from output"
    return 1
}

#
# Prints an error message to the standard error. The text will not mixed up with
# the data that is printed to the standard output.
#
function printError()
{
    local datestring=$(date "+%Y-%m-%d %H:%M:%S")

    echo -e "$*" >&2

    if [ "$LOGFILE" ]; then
        echo -e "$datestring ERROR $MYNAME($$) $*" >>"$LOGFILE"
    fi
}


function cluster_state()
{
    local clusterId="$1"

    s9s cluster --list --cluster-id=$clusterId --cluster-format="%S"
}        

function node_state()
{
    local nodeName="$1"
        
    s9s node --list --batch --long --node-format="%S\n" "$nodeName" | head -n1
}

function node_ip()
{
    local nodeName="$1"
        
    s9s node --list --batch --long --node-format="%A" "$nodeName"
}

function container_ip()
{
    local is_private

    while [ -n "$1" ]; do
        case "$1" in
            --private)
                shift
                is_private="true"
                ;;

            *)
                break
                ;;
        esac
    done


    if [ -z "$is_private" ]; then
        s9s container --list --batch --long --container-format="%A" "$1"
    else
        s9s container --list --batch --long --container-format="%a" "$1"
    fi
}

#
# This function will wait for a node to pick up a state and stay in that state
# for a while.
#
function wait_for_node_state()
{
    local nodeName="$1"
    local expectedState="$2"
    local state
    local waited=0
    local stayed=0

    if [ -z "$nodeName" ]; then
        printError "Expected a node name."
        return 6
    fi

    if [ -z "$expectedState" ]; then 
        printError "Expected state name."
        return 6
    fi

    while true; do
        state=$(node_state "$nodeName")
        if [ "$state" == $expectedState ]; then
            let stayed+=1
        else
            let stayed=0

            #
            # Would be crazy to timeout when we are in the expected state, so we
            # do check the timeout only when we are not in the expected state.
            #
            if [ "$waited" -gt 120 ]; then
                failure "Node $nodeName failed to reach state $expectedState."
                return 1
            fi
        fi

        if [ "$stayed" -gt 10 ]; then
            success "  o Node $nodeName is in state $state, ok."
            return 0
        fi

        let waited+=1
        sleep 1
    done

    return 2
}

function haproxy_node_name()
{
    s9s node --list --long --batch |\
        grep '^h' | \
        awk '{print $5 }'
}

function pgbouncer_node_name()
{
    s9s node --list --long --batch |\
        grep '^b' | \
        awk '{print $5 }'
}

function check_number_of_haproxy_nodes()
{
    local numberOfNodes
    local expected="$1"

    mys9s node --list --long

    numberOfNodes=$(s9s node --list --long --batch | grep '^h' | wc -l)
    if [ "$numberOfNodes" -eq "$expected" ]; then
        success "o Number of HaProxy nodes is $numberOfNodes, ok."
    else
        failure "The number of HaProxy nodes should be $expected."
    fi    
}

#
# Finds the name/ip of the maxscale node.
#
function maxscale_node_name()
{
    s9s node --list --long --batch |\
        grep '^x' | \
        awk '{print $5 }'
}

function check_number_of_maxscale_nodes()
{
    local numberOfNodes
    local expected="$1"

    mys9s node --list --long

    numberOfNodes=$(s9s node --list --long --batch | grep '^x' | wc -l)
    if [ "$numberOfNodes" -eq "$expected" ]; then
        success "o Number of MaxScale nodes is $numberOfNodes, ok."
    else
        failure "The number of MaxScale nodes should be $expected."
    fi    
}

function proxysql_node_name()
{
    s9s node --list --long --batch |\
        grep '^y' | \
        awk '{print $5 }'
}

function check_number_of_proxysql_nodes()
{
    local numberOfNodes
    local expected="$1"

    mys9s node --list --long

    numberOfNodes=$(s9s node --list --long --batch | grep '^y' | wc -l)
    if [ "$numberOfNodes" -eq "$expected" ]; then
        success "o Number of ProxySql nodes is $numberOfNodes, ok."
    else
        failure "The number of ProxySql nodes should be $expected."
    fi    
}

function galera_node_name()
{
    local cluster_id=""
    local cluster_id_option=""

    while [ -n "$1" ]; do
        case "$1" in 
            --cluster-id)
                cluster_id="$2"
                cluster_id_option=" --cluster-id=$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    s9s node --list --long --batch$cluster_id_option | \
        grep '^g' | \
        head -n1  | \
        awk '{print $5 }'
}

function postgresql_node_name()
{
    local cluster_id=""
    local cluster_id_option=""

    while [ -n "$1" ]; do
        case "$1" in 
            --cluster-id)
                cluster_id="$2"
                cluster_id_option=" --cluster-id=$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    s9s node --list --long --batch$cluster_id_option | \
        grep '^p' | \
        head -n1  | \
        awk '{print $5 }'
}

# $1: Name of the node.
# Returns the "container_id" property for the given node.
function node_container_id()
{
    local node_name="$1"

    s9s node --list --node-format "%v\n" "$node_name"
}

#
# Checks the container IDs of the specified nodes.
#
function check_container_ids()
{
    local node_ip
    local node_ips
    local container_id
    local filter=""

    while [ "$1" ]; do
        case "$1" in 
            --galera-nodes)
                shift
                filter="^g"
                ;;

            --postgresql-nodes)
                shift
                filter="^p"
                ;;

            --replication-nodes)
                shift
                filter="^s"
                ;;

            *)
                break
                ;;
        esac
    done

    if [ -z "$filter" ]; then
        failure "check_container_ids(): Missing command line option."
        return 1
    fi

    #
    # Checking the container ids.
    #
    node_ips=$(s9s node --list --long --batch | grep "$filter" | awk '{ print $5 }')
    for node_ip in $node_ips; do
        print_title "Checking Node $node_ip"
        begin_verbatim

        container_id=$(node_container_id "$node_ip")
        echo "      node_ip: $node_ip"
        echo " container_id: $container_id"
            
        if [ -z "$container_id" ]; then
            failure "The container ID was not found."
        fi

        if [ "$container_id" == "-" ]; then
            failure "The container ID is '-'."
        fi
        end_verbatim
    done
}

#
# This function waits until the host goes into CmonHostShutDown state and then
# waits if it remains in that state for a while. A timeout is implemented and 
# the return value shows if the node is indeed in the CmonHostShutDown state.
#
function wait_for_node_shut_down()
{
    wait_for_node_state "$1" "CmonHostShutDown"
    return $?
}

#
# This function waits until the host goes into CmonHostOnline state and then
# waits if it remains in that state for a while. A timeout is implemented and 
# the return value shows if the node is indeed in the CmonHostOnline state.
#
function wait_for_node_online()
{
    wait_for_node_state "$1" "CmonHostOnline"
    return $?
}

#
# This function waits until the host goes into CmonHostOffLine state and then
# waits if it remains in that state for a while. A timeout is implemented and 
# the return value shows if the node is indeed in the CmonHostOffLine state.
#
function wait_for_node_offline()
{
    wait_for_node_state "$1" "CmonHostOffLine"
    return $?
}

#
# This function waits until the host goes into CmonHostFailed state and then
# waits if it remains in that state for a while. A timeout is implemented and 
# the return value shows if the node is indeed in the CmonHostFailed state.
#
function wait_for_node_failed()
{
    wait_for_node_state "$1" "CmonHostFailed"
    return $?
}

function wait_for_cluster_state()
{
    local clusterName
    local expectedState
    local state
    local waited=0
    local stayed=0
    local user_option
    local password_option
    local controller_option

    while [ -n "$1" ]; do
        case "$1" in 
            --system)
                shift
                user_option="--cmon-user=system"
                password_option="--password=secret"
                ;;

            --controller)
                controller_option="--controller=$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    clusterName="$1"
    expectedState="$2"

    if [ -z "$clusterName" ]; then
        failure "wait_for_cluster_state(): Expected a cluster name."
        return 6
    else
        success "  o Waiting for cluster $clusterName state, ok."
    fi

    if [ -z "$expectedState" ]; then 
        failure "wait_for_cluster_state(): Expected cluster state name."
        return 6
    else
        success "  o Expecting cluster state $expectedState, ok."
    fi

    while true; do
        state=$(s9s cluster \
            --list \
            --batch \
            --cluster-format="%S" \
            --cluster-name="$clusterName" \
            $user_option \
            $controller_option \
            $password_option)

        #echo "***         state: '$state'" >&2
        #echo "*** expectedState: '$expectedState'" >&2
        if [ "$state" == $expectedState ]; then
            let stayed+=1
        else
            let stayed=0

            #
            # Would be crazy to timeout when we are in the expected state, so we
            # do check the timeout only when we are not in the expected state.
            #
            if [ "$waited" -gt 120 ]; then
                failure "Failed to reach $expectedState state in ${waited}s."
                return 1
            fi
        fi

        if [ "$stayed" -gt 10 ]; then
            success "  o Cluster is in $expectedState, ok."
            return 0
        fi

        let waited+=10
        sleep 10
    done

    return 2
}

#
# returns: True if the cluster reached STARTED state.
#
function wait_for_cluster_started()
{
    wait_for_cluster_state $* "STARTED"
    return $?
}

function get_container_ip()
{
    local container_name="$1"

    s9s container \
        --list \
        --long \
        --batch \
        --cmon-user="system" \
        --password="secret" \
        "$container_name" \
    | \
        awk '{print $6}'
}

#
# $1: the server name
# $2: The username
#
function is_server_running_ssh()
{
    local serverName
    local owner
    local keyfile
    local keyOption
    local isOK
    local option_current_user
    local logfile="/tmp/is_server_running_ssh.log"
    local timeout=3
    local commandline

    while true; do
        case "$1" in 
            --current-user)
                shift
                option_current_user="true"
                ;;

            *)
                break
        esac
    done

    [ -f "$logfile" ] && rm -f "$logfile"

    serverName="$1"
    owner="$2"
    keyfile="$3"

    if [ "$keyfile" ]; then
        keyOption="-i $keyfile"
    fi

    if [ "$option_current_user" ]; then
        commandline="ssh -o ConnectTimeout=$timeout -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet $keyOption \"$owner@$serverName\" 2>\"$logfile\" -- echo OK"
        isOk=$(ssh -o ConnectTimeout=$timeout \
                -o UserKnownHostsFile=/dev/null \
                -o StrictHostKeyChecking=no \
                -o LogLevel=quiet \
                $keyOption \
                "$owner@$serverName" \
                2>"$logfile" -- echo OK)
    else
        commandline="sudo -u $owner -- ssh -o ConnectTimeout=$timeout -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o LogLevel=quiet $keyOption \"$serverName\" 2>\"$logfile\" -- echo OK"
        isOk=$(sudo -u $owner -- \
            ssh -o ConnectTimeout=$timeout \
                -o UserKnownHostsFile=/dev/null \
                -o StrictHostKeyChecking=no \
                -o LogLevel=quiet \
                $keyOption \
                "$serverName" \
                2>"$logfile" -- echo OK)
    fi

    #
    # If we got the OK, we are finished here.
    #
    if [ "$isOk" == "OK" ]; then
        [ -f "$logfile" ] && rm -f "$logfile"
        return 0
    fi

    #
    # Otherwise print the error message and the command.
    #
    echo "SSH command failed."
    echo "  command: $commandline"
    echo "   stderr: "
    echo "----------------------8<--------------------8<-------------------"
    if [ -f "$logfile" ]; then
        cat $logfile
    fi
    echo "----------------------8<--------------------8<-------------------"

    return 1
}

#
# $2: the server name
#
# Waits until the server is accepting SSH connections. There is also a timeout
# implemented here.
#
function wait_for_server_ssh()
{
    local serverName="$1"
    local OWNER="$2"
    local nTry=0
    local nSuccess=0

    while true; do
        if is_server_running_ssh "$serverName" $OWNER; then
            printVerbose "Server '$serverName' is reachable."
            let nSuccess+=1
        else
            printVerbose "Server '$serverName' is not reachable."
            let nSuccess=0

            # 
            if [ "$nTry" -gt 180 ]; then
                printVerbose "Server '$serverName' did not came alive."
                return 1
            fi
        fi

        if [ "$nSuccess" -gt 3 ]; then
            printVerbose "Server '$serverName' is stable."
            return 0
        fi

        sleep 5
        let nTry+=1
    done

    return 0
}

#
# Usage:
#    create_node [OPTION]... [CONTAINER_NAME] 
#
#  --autodestroy    Destroy the node when the script ends.
#  --template NAME  The name of the template to be used.
# 
# Creates and starts a new virtual machine node. Prints the IP address when the
# container is up.
#
function create_node()
{
    local ip
    local retval
    local verbose_option=""
    local option_autodestroy=""
    local container_list_file="/tmp/${MYNAME}.containers"
    local template_option=""
    local os_vendor_option=""
    local os_release_option=""
    local container_name
    local args
    local os_release=""
    local os_vendor=""
    local template_name=""

    args=$(\
    getopt -o h \
        -l "help,verbose,autodestroy,template:,os-vendor:,os-release:" \
        -- "$@")

    if [ $? -ne 0 ]; then
        return 6
    fi

    eval set -- "$args"

    while [ "$1" ]; do
        case "$1" in 
            -h|--help)
                shift
                echo "Help requested from the create_node() function."
                return 1
                ;;

            --autodestroy)
                shift
                option_autodestroy="true"
                ;;

            --template)
                template_option="--template=$2"
                template_name="$2"
                shift 2
                ;;

            --os-vendor)
                if [ -n "$2" ]; then
                    os_vendor_option="--vendor=$2"
                    os_vendor="$2 "
                fi
                shift 2
                ;;

            --os-release)
                if [ -n "$2" ]; then
                    os_release_option="--release=$2"
                    os_release="$2 "
                fi
                shift 2
                ;;

            --verbose)
                verbose_option="--verbose"
                shift
                ;;

            --)
                shift
                break
                ;;

            *)
                break
                ;;
        esac
    done


    if [ "$VERBOSE" ]; then
        verbose_option="--verbose"
    fi

    if [ -z "$CONTAINER_SERVER" ]; then
        failure "The container server is not set."
        return 1
    fi

    container_name=$1

    if [ -n "$template_name" ]; then
        echo -n "Creating container from template $template_name" >&2
    else
        echo -n "Creating container... $os_vendor$os_release" >&2
    fi
    if [ "$PRINT_PIP_COMMANDS" ]; then
        print_command \
            "pip-container-create" \
            $os_vendor_option \
            $os_release_option \
            $template_option \
            $verbose_option \
            --server=$CONTAINER_SERVER $container_name
    fi
    ip=$(pip-container-create \
        $os_vendor_option \
        $os_release_option \
        $template_option \
        $verbose_option \
        --server=$CONTAINER_SERVER $container_name)

    retval=$?
    if [ "$retval" -ne 0 ]; then
        failure "pip-container-create returned ${retval}."
        #tail $HOME/pip-container-create.log >&2
    #else
    #    success "pip-container-create returned ${retval}."
    fi

    printVerbose "Created '$ip'."
   
    #
    # Waiting until the server is up and accepts SSH connections.
    #
#    wait_for_server_ssh "$ip" "$USER"
#    retval=$?
#    if [ "$retval" -ne 0 ]; then
#        echo -e " $XTERM_COLOR_RED[FAILURE]$TERM_NORMAL" >&2
#        echo "Could not reach created server at ip '$ip'." >&2
#    else
#        echo -e " $XTERM_COLOR_GREEN[SUCCESS]$TERM_NORMAL" >&2
#    fi

    if [ "$option_autodestroy" ]; then
        echo "$ip" >>$container_list_file
    fi

    #
    # If we are here we seem to be having a new IP address.
    #
    if [ -z "$FIRST_ADDED_NODE" ]; then
        FIRST_ADDED_NODE="$ip"
    elif [ -z "$SECOND_ADDED_NODE" ]; then
        SECOND_ADDED_NODE="$ip"
    elif [ -z "$THIRD_ADDED_NODE" ]; then
        THIRD_ADDED_NODE="$ip"
    elif [ -z "$FOURTH_ADDED_NODE" ]; then
        FOURTH_ADDED_NODE="$ip"
    fi
   
    LAST_ADDED_NODE="$ip"
    echo $ip
}

function node_created()
{
    local container_ip="$1"
    local container_list_file="/tmp/${MYNAME}.containers"
    
    echo "$container_ip" >>"$container_list_file"
}

function emit_s9s_configuration_file()
{
    local hostname="localhost"
    local cmon_port="$OPTION_CMON_PORT"

    [ -z "$cmon_port" ] && cmon_port="9556"
    

    while [ -n "$1" ]; do
        case "$1" in
            --do-not-create)
                # No need to do anything with this option here. If we are here
                # we create the configuration anyway.
                ;;

            --controller)
                hostname="$2"
                shift 2
                ;;

            --port)
                cmon_port="$2"
                shift 2
                ;;

            *)
                failure "emit_s9s_configuration_file: Unknown option '$1'."
                return 1;
        esac
    done

    cat <<EOF
#
# This configuration file was created by ${MYNAME} version ${VERSION}.
#
[global]
controller    = https://$hostname:$cmon_port

[network]
client_connection_timeout = 30

[log]
brief_job_log_format = "%36B:%-5L: %-7S %M\n"
brief_log_format     = "%C %36B:%-5L: %-8S %M\n"

#brief_job_log_format = "%-8S %M\n"
#brief_log_format     = "%-8S %M\n"

#brief_job_log_format = "%C %-8S %M\n"
#brief_log_format     = "%C %-8S %M\n"


log_file             = "$HOME/s9s.log"

EOF
}

#
# Just for backward compatibility.
#
function reset_config()
{
    create_s9s_config $*
}

#
# Creates an s9s configuration file that holds the information about the
# controller. This is crutial for every tests.
# 
# If the S9S_USER_CONFIG variable is not empty that holds the full path of the
# configura6tion file (the s9s program understands this variable too). If the
# S9S_USER_CONFIG variable is empty the ~/.s9s/s9s.conf path will be used (that
# is the default for the s9s program too).
#
function create_s9s_config()
{
    local config_dir
    local config_file
    local do_not_create
    local options=$*

    if [ -z "$S9S_USER_CONFIG" ]; then
        config_dir="$HOME/.s9s"
        config_file="$config_dir/s9s.conf"
    else
        config_file="$S9S_USER_CONFIG"
        config_dir="$(dirname "$config_file")"
    fi

    while [ "$1" ]; do
        case "$1" in 
            --do-not-create)
                shift
                do_not_create="true"
                ;;

            --controller)
                # No need to do anything with this option here.
                shift 2
                ;;

            --port)
                # No need to do anything with this option here.
                shift 2
                ;;

            *)
                break
        esac
    done

    if [ -z "$OPTION_RESET_CONFIG" ]; then
        return 0
    fi
   
    if [ -z "$do_not_create" ]; then
        print_title "Writing s9s Configuration $config_file"
    else
        print_title "Deleting s9s Configuration"
    fi

    if [ -z "$do_not_create" ]; then
        if [ ! -d "$config_dir" ]; then
            mkdir "$config_dir"
        fi

        emit_s9s_configuration_file $options >$config_file

        # This goes to the standard output.
        emit_s9s_configuration_file $options | print_ini_file
    fi

    # FIXME: This should not be here:
    sudo rm -f $HOME/pip-container-create.log 2>/dev/null
}

function remove_cmon_configs()
{
    print_title "Checking Remaining Cmon Configs & Logs"
    rm -rf /tmp/cmon*
}

#
# $1: the name of the cluster
# returns: True if the cluster found, false if not.
#
function find_cluster_id()
{
    local name="$1"
    local retval
    local nTry=0
    local password_option=""
    
    if [ -n "$CMON_USER_PASSWORD" ]; then
        password_option="--password='$CMON_USER_PASSWORD'"
    fi

    while true; do
        retval=$($S9S cluster \
            --list \
            --long \
            --batch \
            $password_option \
            --cluster-name="$name")

        retval=$(echo "$retval" | awk '{print $1}')

        if [ -z "$retval" ]; then
            let nTry+=1

            if [ "$nTry" -gt 10 ]; then
                # Not found, timeout reached.
                failure "  o Cluster $name was not found, giving up."
                echo "NOT-FOUND"
                return 1
            else
                # Not found, still waiting.
                printVerbose "Cluster '$name' was not found, waiting..."
                sleep 3
            fi
        else
            # Cluster with the given name found.
            printVerbose "Cluster '$name' was found with ID ${retval}."
            echo "$retval"
            return 0
        fi
    done
}

#
# $1: the name of the cluster
#
function get_mysql_root_password()
{
    local name="$1"
    local retval
    local nTry=0
    local password_option=""

    if [ -n "$CMON_USER_PASSWORD" ]; then
        password_option="--password='$CMON_USER_PASSWORD'"
    fi

    while true; do
        retval=$($S9S cluster \
            --list-config \
            --batch \
            --color=none \
            $password_option \
            --cluster-name="$name")

        retval=$(echo "$retval" | \
            grep monitored_mysql_root_password | \
            cut -d'"' -f2 )

        if [ -z "$retval" ]; then
            printVerbose "Cluster '$name' was not found."
            let nTry+=1

            if [ "$nTry" -gt 10 ]; then
                echo "NOT-FOUND"
                break
            else
                sleep 3
            fi
        else
            printVerbose "Cluster '$name' was found with ID ${retval}."
            echo "$retval"
            break
        fi
    done
}

#
# Just a normal createUser call we do all the time to register a user on the
# controller so that we can actually execute RPC calls.
#
function grant_user()
{
    local first
    local last
    local cmon_port="$OPTION_CMON_PORT"

    [ -z "$cmon_port" ] && cmon_port="9556"

    print_title "Creating the First User"
    cat <<EOF | paragraph
  This is where we create the initial Cmon user. It is the initial because there
  is no user defined in the s9s configuration file and in the command line
  either. When this happens the s9s will access the controller through SSH and
  push the user creation command through a named pipe the Cmon Controller
  provided for exactly this reason. So there is a passwordless ssh and
  a passwordless sudo is behind this. If these are not possible you need to ssh
  to the controller host and do this as root.

  After the initial user created we also register a public key so that the Cmon
  user account can be used without password. 
EOF

    begin_verbatim

    first=$(getent passwd $USER | cut -d ':' -f 5 | cut -d ',' -f 1 | cut -d ' ' -f 1)
    last=$(getent passwd $USER | cut -d ':' -f 5 | cut -d ',' -f 1 | cut -d ' ' -f 2)

    mys9s user \
        --create \
        --group="testgroup" \
        --create-group \
        --generate-key \
        --controller="https://localhost:$cmon_port" \
        --new-password="p" \
        --email-address=${EMAIL} \
        --first-name="$first" \
        --last-name="$last" \
        $OPTION_PRINT_JSON \
        $OPTION_VERBOSE \
        --batch \
        "$USER"

    exitCode=$?
    if [ "$exitCode" -ne 0 ]; then
        failure "Exit code is not 0 while granting user."
    else 
        success "  o Return code is 0, ok."
    fi

    mys9s user --stat "$USER"
    if [ $? -eq 0 ]; then
        success "  o Could stat the user $USER, OK."
    else
        failure "Exit code is not 0 when stat-ing the new user $USER."
        end_verbatim
        return 0
    fi

    for file in \
        "/.runtime/jobs/jobExecutor" \
        "/.runtime/jobs/jobExecutorCreateCluster" \
        "/.runtime/jobs/jobExecutorDeleteOldJobs"
    do
        mys9s tree \
            --add-acl \
            --acl="user:$USER:r-x" \
            --cmon-user="system" \
            --password="secret" \
            $file
    done

    # Adding a tag to the user and checking if the tag is indeed added.
    mys9s tree --add-tag --tag="testUser" /$USER
    mys9s user --stat $USER

    if s9s user --stat $USER | grep -q testUser; then
        success "  o User $USER has the tag 'testUser' set, OK."
    else
        warning "  o User $USER has no tag 'testUser' set."
    fi

    #
    # Adding the user's default SSH public key. This will come handy when we
    # create a container because this way the user will be able to log in with
    # the SSH key without password.
    #
    mys9s user \
        --add-key \
        --batch \
        --password="p" \
        --public-key-file="/home/$USER/.ssh/id_rsa.pub" \
        --public-key-name="The_SSH_key"

    end_verbatim

    #
    #
    #
    print_subtitle "The $HOME/.s9s/s9s.conf"
    cat <<EOF | paragraph
  The creation of the initial user changes the configuration file. The s9s
  program automatically puts the username into the configuration file so that
  next time it can be used without passing the account name in the command line.

EOF

    cat $HOME/.s9s/s9s.conf | print_ini_file
}

function get_user_group()
{
    local user_name="$1"

    if [ -z "$user_name" ]; then
        failure "No user name in get_user_group()"
        return 1
    fi

    s9s user --list --user-format="%G" $user_name
}

function get_user_email()
{
    local user_name="$1"

    if [ -z "$user_name" ]; then
        failure "No user name in get_user_group()"
        return 1
    fi

    s9s user --list --user-format="%M" $user_name
}

function check_controller()
{
    local owner
    local group
    local cdt_path
    local status
    local tmp

    while [ -n "$1" ]; do
        case "$1" in 
            --owner)
                owner="$2"
                shift 2
                ;;

            --group)
                group="$2"
                shift 2
                ;;

            --cdt-path)
                cdt_path="$2"
                shift 2
                ;;

            --status)
                status="$2"
                shift 2
                ;;
        esac
    done

    echo ""
    echo "Checking controller..."

    if [ -n "$owner" ]; then
        tmp=$(\
            s9s node --list --node-format "%R %O\n" | \
            grep "controller" | \
            awk '{print $2}')

        if [ "$tmp" == "$owner" ]; then
            success "  o The owner of the controller is $tmp, ok."
        else
            failure "The owner of the controller should not be '$tmp'."
        fi
    fi

    if [ -n "$group" ]; then
        tmp=$(\
            s9s node --list --node-format "%R %G\n" | \
            grep "controller" | \
            awk '{print $2}')

        if [ "$tmp" == "$group" ]; then
            success "  o The group of the controller is $tmp, ok."
        else
            failure "The group of the controller should not be '$tmp'."
        fi
    fi

    if [ -n "$cdt_path" ]; then
        tmp=$(\
            s9s node --list --node-format "%R %h\n" | \
            grep "controller" | \
            awk '{print $2}')

        if [ "$tmp" == "$cdt_path" ]; then
            success "  o The CDT path of the controller is $tmp, ok."
        else
            failure "The CDT path of the controller should not be '$tmp'."
        fi

    fi

    if [ -n "$status" ]; then
        tmp=$(\
            s9s node --list --node-format "%R %S\n" | \
            grep "controller" | \
            awk '{print $2}')

        if [ "$tmp" == "$status" ]; then
            success "  o The status of the controller is $tmp, ok."
        else
            failure "The status of the controller should not be '$tmp'."
        fi

    fi
}

function check_node()
{
    local hostname
    local ipaddress
    local port
    local owner
    local group
    local cdt_path
    local status
    local config
    local config_basename
    local no_maintenance
    local tmp

    while [ -n "$1" ]; do
        case "$1" in 
            --node|--host)
                hostname="$2"
                shift 2
                ;;

            --ip-address)
                ipaddress="$2"
                shift 2
                ;;

            --port)
                port="$2"
                shift 2
                ;;

            --owner)
                owner="$2"
                shift 2
                ;;

            --group)
                group="$2"
                shift 2
                ;;

            --cdt-path)
                cdt_path="$2"
                shift 2
                ;;

            --status)
                status="$2"
                shift 2
                ;;

            --config)
                config="$2"
                shift 2
                ;;

            --config-basename)
                config_basename="$2"
                shift 2
                ;;

            --no-maint|--no-maintenance)
                no_maintenance="true"
                shift
                ;;
        esac
    done

    if [ -z "$hostname" ]; then
        failure "Hostname is not provided while checking node."
        return 1
    fi

    echo ""
    echo "Checking node '$hostname'..."

    if [ -n "$ipaddress" ]; then
        tmp=$(s9s node --list --batch --node-format "%A\n" "$hostname")

        if [ "$tmp" == "$ipaddress" ]; then
            success "  o The IP of the node is $tmp, ok."
        else
            failure "The IP of the node should not be '$tmp'."
        fi
    fi
    
    if [ -n "$port" ]; then
        tmp=$(s9s node --list --batch --node-format "%P\n" "$hostname")

        if [ "$tmp" == "$port" ]; then
            success "  o The port of the node is $tmp, ok."
        else
            failure "The port of the node should not be '$tmp'."
        fi
    fi

    if [ -n "$owner" ]; then
        tmp=$(s9s node --list --batch --node-format "%O\n" "$hostname")

        if [ "$tmp" == "$owner" ]; then
            success "  o The owner of the node is $tmp, ok."
        else
            failure "The owner of the node should not be '$tmp'."
        fi
    fi

    if [ -n "$group" ]; then
        tmp=$(s9s node --list --batch --node-format "%G\n" "$hostname")

        if [ "$tmp" == "$group" ]; then
            success "  o The group of the node is $tmp, ok."
        else
            failure "The group of the node should not be '$tmp'."
        fi
    fi

    if [ -n "$cdt_path" ]; then
        tmp=$(s9s node --list --batch --node-format "%h\n" "$hostname")

        if [ "$tmp" == "$cdt_path" ]; then
            success "  o The CDT path of the node is $tmp, ok."
        else
            failure "The CDT path of the node should not be '$tmp'."
        fi

    fi

    if [ -n "$status" ]; then
        tmp=$(s9s node --list --batch --node-format "%S\n" "$hostname")

        if [ "$tmp" == "$status" ]; then
            success "  o The status of the node is $tmp, ok."
        else
            failure "The status of the node should not be '$tmp'."
        fi
    fi
    
    if [ -n "$config" ]; then
        tmp=$(s9s node --list --batch --node-format "%C\n" "$hostname")

        if [ "$tmp" == "$config" ]; then
            success "  o The config file of the node is $tmp, ok."
        else
            failure "The config file of the node should not be '$tmp'."
        fi
    fi
    
    if [ -n "$config_basename" ]; then
        tmp=$(s9s node --list --batch --node-format "%C\n" "$hostname")
        tmp=$(basename "$tmp")

        if [ "$tmp" == "$config_basename" ]; then
            success "  o The basename of config file of the node is $tmp, ok."
        else
            failure "The basename of config file should not be '$tmp'."
        fi
    fi
    
    if [ -n "$no_maintenance" ]; then
        tmp=$(s9s node --list --batch --node-format "%a\n" "$hostname")

        if [ "$tmp" == "-" ]; then
            success "  o The maintenance of the node is '$tmp', ok."
        else
            failure "The maintenance of the node should not be '$tmp'."
        fi
    fi
}

function check_cluster()
{
    local config_file
    local log_file
    local owner
    local group
    local cdt_path
    local cluster_type
    local cluster_name
    local cluster_state
    local tmp

    while [ -n "$1" ]; do
        case "$1" in 
            --cluster|--cluster-name)
                cluster_name="$2"
                shift 2
                ;;

            --config)
                config_file="$2"
                shift 2
                ;;

            --log)
                log_file="$2"
                shift 2
                ;;
                
            --owner)
                owner="$2"
                shift 2
                ;;

            --group)
                group="$2"
                shift 2
                ;;

            --cdt-path)
                cdt_path="$2"
                shift 2
                ;;

            --type|--cluster-type)
                cluster_type="$2"
                shift 2
                ;;

            --state|--cluster-state)
                cluster_state="$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    if [ -z "$cluster_name" ]; then
        failure "No cluster name provided while checking cluster."
        return 1
    else
        success "  o Will check state of cluster '$cluster_name', ok"
    fi

    if [ -n "$config_file" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%C\n" "$cluster_name")

        if [ "$tmp" == "$config_file" ]; then
            success "  o The config file of the cluster is $tmp, ok."
        else
            failure "The config file of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$log_file" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%L\n" "$cluster_name")

        if [ "$tmp" == "$log_file" ]; then
            success "  o The log file of the cluster is $tmp, ok."
        else
            failure "The log file of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$owner" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%O\n" "$cluster_name")

        if [ "$tmp" == "$owner" ]; then
            success "  o The owner of the cluster is $tmp, ok."
        else
            failure "The owner of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$group" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%G\n" "$cluster_name")

        if [ "$tmp" == "$group" ]; then
            success "  o The group of the cluster is $tmp, ok."
        else
            failure "The group of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$cdt_path" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%P\n" "$cluster_name")

        if [ "$tmp" == "$cdt_path" ]; then
            success "  o The CDT path of the cluster is $tmp, ok."
        else
            failure "The CDT path of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$cluster_type" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%T\n" "$cluster_name")

        if [ "$tmp" == "$cluster_type" ]; then
            success "  o The type of the cluster is $tmp, ok."
        else
            failure "The type of the cluster should not be '$tmp'."
        fi
    fi

    if [ -n "$cluster_state" ]; then
        tmp=$(s9s cluster --list --batch --cluster-format "%S\n" "$cluster_name")

        if [ "$tmp" == "$cluster_state" ]; then
            success "  o The state of the cluster is $tmp, ok."
        else
            failure "The state of the cluster should not be '$tmp'."
        fi
    fi

}

function check_replication_state()
{
    local cluster_name    
    local state
    local expected_state
    local slave

    while [ -n "$1" ]; do
        case "$1" in 
            --cluster|--cluster-name)
                cluster_name="$2"
                shift 2
                ;;
           
            --state)
                expected_state="$2"
                shift 2
                ;;

            --slave)
                slave="$2"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    if [ -n "$expected_state" -a -n "$slave" ]; then
        success "  o Will check replication on slave $slave, ok."
    else
        failure "check_replication_state(): Invalid arguments."
        return 1
    fi

    state=$(s9s replication --list --batch --link-format="%s\n" --slave=$slave)
    if [ "$state" == "$expected_state" ]; then
        success "  o Replication state on $slave is $state, ok."
    else
        failure "Replication state is '$state', should be '$expected_state'."
        mys9s replication --list --long
        mys9s replication --list --long --slave=$slave
        return 1
    fi

    return 0
}

function check_group()
{
    local group
    local owner_name
    local group_owner_name
    local tmp

    while [ -n "$1" ]; do
        case "$1" in 
            --group-name)
                group="$2"
                shift 2
                ;;
           
            --owner-name)
                owner_name="$2"
                shift 2
                ;;

            --group-owner)
                group_owner_name="$2"
                shift 2
                ;;
            
            *)
                printError "check_user(): Invalid option '$1'."
                return 1
                break
                ;;
        esac
    done
    
    mys9s user --list-groups --long 

    if [ -n "$group" ]; then
        success "  o received group name '$group' to test, ok"
    else
        failure "check_group(): No group name provided."
        return 1
    fi

    if [ -n "$owner_name" ]; then
        tmp=$(s9s user --list-groups --long --batch $group | awk '{print $2}')

        if [ "$tmp" == "$owner_name" ]; then 
            success "  o owner of the group is $tmp, ok"
        else
            failure "The owner of the group is $tmp."
        fi
    fi
    
    if [ -n "$group_owner_name" ]; then
        tmp=$(s9s user --list-groups --long --batch $group | awk '{print $3}')

        if [ "$tmp" == "$group_owner_name" ]; then 
            success "  o owner of the group is $tmp, ok"
        else
            failure "The owner of the group is $tmp."
        fi
    fi
}

function check_user()
{
    local user_name
    local group
    local password
    local email
    local check_key
    local tmp
    local option_origin
    local option_cdt_path
    local option_dn
    local option_full_name

    while [ -n "$1" ]; do
        case "$1" in 
            --user-name)
                user_name="$2"
                shift 2
                ;;

            --full-name)
                option_full_name="$2"
                shift 2
                ;;
                
            --dn|--distinguished-name)
                option_dn="$2"
                shift 2
                ;;

            --group)
                group="$2"
                shift 2
                ;;

            --email-address|--email)
                email="$2"
                shift 2
                ;;

            --origin)
                option_origin="$2"
                shift 2
                ;;

            --cdt-path)
                option_cdt_path="$2"
                shift 2
                ;;

            --password)
                password="$2"
                shift 2
                ;;

            --check-key)
                check_key="true"
                shift
                ;;

            *)
                printError "check_user(): Invalid option '$1'."
                return 1
                break
                ;;
        esac
    done

    echo "" 

    if [ -n "$user_name" ]; then
        success "  o received user name '$user_name' to test, ok"
    else
        failure "check_user(): No username provided."
        return 1
    fi

    #
    # If the group is provided we check the group of the user.
    #
    if [ -n "$group" ]; then
        tmp=$(get_user_group "$user_name")
        if [ "$tmp" != "$group" ]; then
            failure "The group be '$group' and not '$tmp'."
        else
            success "  o group is '$group', ok"
        fi
    fi

    #
    # Checking the authentication with a password.
    #
    if [ -n "$password" ]; then
        tmp=$(s9s user --whoami --batch --cmon-user="$user_name" --password="$password")
        if [ "$tmp" != "$user_name" ]; then
            failure "User $user_name can not log in with password."
        else
            success "  o login with password ok"
        fi
    fi

    #
    # Checking the login with the key.
    #
    if [ -n "$check_key" ]; then
        tmp=$(s9s user --whoami --batch --cmon-user="$user_name")
        if [ "$tmp" != "$user_name" ]; then
            failure "User $user_name can not log in with key."
        else
            success "  o login with key ok"
        fi

        tmp="$HOME/.s9s/${user_name}.key"
        if [ ! -f "$tmp" ]; then
            failure "File $tmp does not exist."
        else
            success "  o file '$tmp' exists, ok"
        fi
        
        tmp="$HOME/.s9s/${user_name}.pub"
        if [ ! -f "$tmp" ]; then
            failure "File $tmp does not exist."
        else
            success "  o file '$tmp' exists, ok"
        fi

        #mys9s user --list-keys --cmon-user=$user_name
    fi

    if [ -n "$option_full_name" ]; then
        tmp=$(s9s user --list --batch --user-format="%F\n" "$user_name" | tr ' ' '_')
        tmp=$(echo "$tmp" | tr '_' ' ')
        if [ "$tmp" != "$option_full_name" ]; then
            failure "The full name should be name $option_full_name, not $tmp."
        else
            success "  o the full name is '$tmp', ok"
        fi
    fi

    #
    # Checking the email address if we are requested to do so.
    #
    if [ -n "$email" ]; then
        tmp=$(get_user_email "$user_name")
        if [ "$tmp" != "$email" ]; then
            failure "The email of $user_name should be $email and not $tmp."
        else
            success "  o email address is $email, ok"
        fi
    fi

    if [ -n "$option_dn" ]; then
        tmp=$(s9s user --list --batch --user-format="%d\n" "$user_name")
        if [ "$tmp" != "$option_dn" ]; then
            failure "Distinguished name should be name $option_dn, not $tmp."
        else
            success "  o the distinguished name is '$tmp', ok"
        fi
    fi

    if [ -n "$option_origin" ]; then
        tmp=$(s9s user --list --batch --user-format="%o\n" "$user_name")
        if [ "$tmp" != "$option_origin" ]; then
            failure "The origin should be '$option_origin' and not '$tmp'."
        else
            success "  o the origin is '$tmp', ok"
        fi
    fi
    
    if [ -n "$option_cdt_path" ]; then
        tmp=$(s9s user --list --batch --user-format="%P\n" "$user_name")
        if [ "$tmp" != "$option_cdt_path" ]; then
            failure "The CDT path should be '$option_cdt_path' and not '$tmp'."
        else
            success "  o the CDT path is '$tmp', ok"
        fi
    fi
}

#
# A flexible function to check the properties and the state of a container
# server.
#
function check_container_server()
{
    local container_server
    local expected_class_name
    local old_ifs="$IFS"
    local n_names_found
    local class
    local cloud
    local cloud_option
    local file

    while [ -n "$1" ]; do
        case "$1" in 
            --server-name)
                container_server="$2"
                shift 2
                ;;

            --class)
                expected_class_name="$2"
                shift 2
                ;;

            --cloud)
                cloud="$2"
                cloud_option="--cloud=$cloud"
                shift 2
                ;;

            *)
                break
                ;;
        esac
    done

    #
    #
    #
    print_title "Checking Server $container_server"
    
    begin_verbatim 
    if [ -z "$container_server" ]; then
        failure "check_container_server(): No server name."
        return 1
    fi

    #mys9s server --list --long $container_server
    #mys9s server --stat        $container_server

    #
    # Checking the class is very important.
    #
    class=$(\
        s9s server --stat "$container_server" \
        | grep "Class:" | awk '{print $2}')

    if [ -n "$expected_class_name" ]; then
        if [ "$class" != "$expected_class_name" ]; then
            failure "Server $container_server has '$class' class."
        fi
    elif [ -z "$class" ]; then
        failure "Server $container_server has empty class name."
    fi

    #
    # Checking the state runtime information.
    #
    echo ""
    file="/$container_server/.runtime/state"
    n_names_found=0
    
    mys9s tree --cat $file
    
    IFS=$'\n'
    for line in $(s9s tree --cat --batch $file)
    do
        name=$(echo "$line" | awk '{print $1}')
        value=$(echo "$line" | awk '{print substr($0, index($0,$3))}')
        printf "$XTERM_COLOR_BLUE%32s$TERM_NORMAL is " "$name"
        printf "'$XTERM_COLOR_ORANGE%s$TERM_NORMAL'\n" "$value"
        
        [ -z "$name" ]  && failure "Name is empty."
        [ -z "$value" ] && failure "Value is empty for $name."
        case "$name" in
            container_server_instance)
                let n_names_found+=1
                ;;

            container_server_class)
                [ "$value" != "$expected_class_name" ] && \
                    failure "Value is '$value'."
                let n_names_found+=1
                ;;

            server_name)
                [ "$value" != "$container_server" ] && \
                    failure "Value is '$value'."
                let n_names_found+=1
                ;;

            number_of_processors)
                if [ "$expected_class_name" != "CmonCloudServer" ]; then
                    [ "$value" -lt 1 ] && \
                        failure "Value is less than 1."
                fi

                let n_names_found+=1
                ;;

            number_of_cpu_threads)
                if [ "$expected_class_name" != "CmonCloudServer" ]; then
                    [ "$value" -lt 1 ] && \
                        failure "Value is less than 1."
                fi

                let n_names_found+=1
                ;;
            
            total_memory_gbyte)
                if [ "$expected_class_name" != "CmonCloudServer" ]; then
                    [ "$value" -lt 2 ] && \
                        failure "Value is less than 2."
                fi

                let n_names_found+=1
                ;;
        esac
    done 
    IFS=$old_ifs

    #echo "n_names_found: $n_names_found"
    #echo 
    if [ "$n_names_found" -lt 6 ]; then
        failure "Some lines could not be found in $file."
    fi

    #
    # Checking the server manager.
    #
    echo ""
    file="/.runtime/server_manager"
    n_names_found=0

    #mys9s tree \
    #    --cat \
    #    --cmon-user=system \
    #    --password=secret \
    #    $file

    IFS=$'\n'
    for line in $(s9s tree --cat --batch --cmon-user=system --password=secret $file)
    do
        name=$(echo "$line" | awk '{print $1}')
        value=$(echo "$line" | awk '{print $3}')
        printf "$XTERM_COLOR_BLUE%32s$TERM_NORMAL is " "$name"
        printf "'$XTERM_COLOR_ORANGE%s$TERM_NORMAL'\n" "$value"
        
        [ -z "$name" ]  && failure "Name is empty."
        [ -z "$value" ] && failure "Value is empty for $name."
        case "$name" in 
            server_manager_instance)
                let n_names_found+=1
                ;;
            
            number_of_servers)
                [ "$value" -lt 1 ] && \
                    failure "Value is less than 1."
                let n_names_found+=1
                ;;

            number_of_processors)
                if [ "$expected_class_name" != "CmonCloudServer" ]; then
                    [ "$value" -lt 1 ] && \
                        failure "Value is less than 1."
                fi

                let n_names_found+=1
                ;;

            number_of_processor_threads)
                if [ "$expected_class_name" != "CmonCloudServer" ]; then
                    [ "$value" -lt 1 ] && \
                        failure "Value is less than 1."
                fi

                let n_names_found+=1
                ;;

            total_memory_gbyte)
                if [ "$expected_class_name" != "CmonCloudServer" ]; then
                    [ "$value" -lt 2 ] && \
                        failure "Value is less than 2."
                fi

                let n_names_found+=1
                ;;
        esac 
    done 
    IFS=$old_ifs
    
    #echo "n_names_found: $n_names_found"
    #echo 

    if [ "$n_names_found" -lt 5 ]; then
        failure "Some lines could not be found in $file."
    fi

    #
    # Checking the regions.
    #
    mys9s server --list-regions $cloud_option

    n_names_found=0
    IFS=$'\n'
    for line in $(s9s server --list-regions --batch $cloud_option)
    do
        #echo "Checking line $line"
        the_credentials=$(echo "$line" | awk '{print $1}')
        the_cloud=$(echo "$line" | awk '{print $2}')
        the_server=$(echo "$line" | awk '{print $3}')
        the_region=$(echo "$line" | awk '{print $4}')

        if [ "$the_server" != "$container_server" ]; then
            continue
        fi

        if [ "$the_credentials" != 'Y' ]; then
            continue
        fi

        let n_names_found+=1
    done
    IFS=$old_ifs
    
    if [ "$n_names_found" -lt 1 ]; then
        failure "No regions with credentials found."
        failure "No regions means the cloud is not operational!"
        end_verbatim
        return 1
    fi

    #
    # Checking the templates.
    #
    #mys9s server --list-templates --long $cloud_option

    n_names_found=0
    IFS=$'\n'
    for line in $(s9s server --list-templates --batch --long $cloud_option)
    do
        #echo "Checking line $line"
        line=$(echo "$line" | sed -e 's/Southeast Asia/Southeast_Asia/g')
        the_cloud=$(echo "$line" | awk '{print $1}')
        the_region=$(echo "$line" | awk '{print $2}')
        the_server=$(echo "$line" | awk '{print $5}')
        the_template=$(echo "$line" | awk '{print $6}')

        #echo "cloud: '$the_cloud' region: '$the_region' server: '$the_server'"
        if [ "$the_server" != "$container_server" ]; then
            continue
        fi

        if [ -n "$cloud" ]; then
            if [ "$the_cloud" != "$cloud" ]; then
                failure "The cloud is $the_cloud is not $cloud."
            fi
        fi

        if [ -z "$the_template" ]; then
            failure "Template name is missing."
        fi

        let n_names_found+=1
    done
    IFS=$old_ifs

    if [ "$n_names_found" -lt 1 ]; then
        failure "No templates found."
    fi

    end_verbatim
}

function check_entry()
{
    local line
    local full_path
    local path
    local owner
    local expected_owner
    local group
    local expected_group
    local acl
    local expected_acl
    local size
    local expected_size

    while [ -n "$1" ]; do
        case "$1" in
            --owner|--user)
                expected_owner="$2"
                shift 2
                ;;

            --group)
                expected_group="$2"
                shift 2
                ;;
            
            --acl)
                expected_acl="$2"
                shift 2
                ;;

            --size)
                expected_size="$2"
                shift 2
                ;;

            *)
                break
                ;;

        esac
    done

    full_path="$1"
    
    #mys9s tree --list --long --full-path --recursive --directory --all \
    #    $full_path

    line=$(s9s tree \
        --list --long --full-path --recursive --directory --batch --all \
        $full_path)

    path=$(echo $line | awk '{print $5}')
    group=$(echo $line | awk '{print $4}')
    owner=$(echo $line | awk '{print $3}')
    size=$(echo $line | awk '{print $2}')
    acl=$(echo $line | awk '{print $1}')


    echo "Checking CDT entry $full_path:"
    if [ "$path" != "$full_path" ]; then
        failure "Entry '$full_path' was not found."
        return 1
    else
        success "  o Entry '$full_path' was found, ok."
    fi

    if [ -n "$expected_group" -a "$group" != "$expected_group" ]; then
        failure "Group is '$group' instead of '$expected_group'."
    else
        success "  o Group is '$group', ok."
    fi
    
    if [ -n "$expected_owner" -a "$owner" != "$expected_owner" ]; then
        failure "Owner is '$owner' instead of '$expected_owner'."
    else
        success "  o Owner is '$owner', ok."
    fi
    
    if [ -n "$expected_acl" -a "$acl" != "$expected_acl" ]; then
        failure "Acl is '$acl' instead of '$expected_acl'."
    else
        success "  o Acl is '$acl', ok."
    fi

    if [ -n "$expected_size" -a "$size" != "$expected_size" ]; then
        failure "Size is '$size' instead of '$expected_size'."
    else
        success "  o Size is '$size', ok."
    fi

}

#
# This will destroy the containers we created. This method is automatically
# called by this:
#
# trap clean_up_after_test EXIT
#
function clean_up_after_test()
{
    local all_created_ip=""
    local container_list_file="/tmp/${MYNAME}.containers"
    local container

    #
    # Some closing logs.
    #
    print_subtitle "Preparing to Exit"
    cat <<EOF
  The clean_up_after_test method is running now to clean up after the test
  script.
EOF

    if false; then
        mys9s tree \
            --cat \
            --cmon-user=system \
            --password=secret \
            /.runtime/jobs/job_manager
    
        mys9s tree \
            --cat \
            --cmon-user=system \
            --password=secret \
            /.runtime/jobs/host_manager
    fi

    # Reading the container list file.
    if [ -f "$container_list_file" ]; then
        for container in $(cat $container_list_file); do
            if [ -z "$container" ]; then
                continue
            fi

            if [ "$all_created_ip" ]; then
                all_created_ip+=" "
            fi

            all_created_ip+="$container"
        done
    fi

    # Destroying the nodes if we have to.
    if [ "$OPTION_KEEP_NODES" ]; then
        print_subtitle "Leaving the Containers"
        echo "The --keep-nodes option was provided, not destroying the "
        echo "containers."
        
        begin_verbatim
        echo "     server : $CONTAINER_SERVER"
        echo " containers : $all_created_ip"
        end_verbatim
    elif [ "$OPTION_INSTALL" ]; then
        print_subtitle "Leaving the Containers"
        echo "The --install option was provided, not destroying the "
        echo "containers."
        
        begin_verbatim
        echo "     server : $CONTAINER_SERVER"
        echo " containers : $all_created_ip"
        end_verbatim
    elif [ "$all_created_ip" ]; then
        print_subtitle "Destroying the Containers"
        
        begin_verbatim
        echo "     server : $CONTAINER_SERVER"
        echo " containers : $all_created_ip"

        pip-container-destroy \
            --server=$CONTAINER_SERVER \
            "$all_created_ip" \
            >/dev/null 2>/dev/null
        
        echo "    retcode : $?"
        end_verbatim
    fi

    # Destroying the container list file.
    if [ -f "$container_list_file" ]; then
        rm -f "$container_list_file"
    fi

    if [ -f "$HOME/s9s.log" ]; then
        rm -f "$HOME/s9s.log"
    fi

    #
    # Uninstalling cmon-cloud.
    #
    if [ -f "/etc/init.d/cmon-cloud" ]; then
        echo "Removing cmon-cloud if installed."
    
        echo "# sudo systemctl stop cmon-cloud"
        sudo systemctl stop cmon-cloud
        sleep 5
    
        echo "# sudo systemctl status cmon-cloud"
        sudo systemctl status cmon-cloud

        echo "# ps aux | grep cmon-cloud"
        ps aux | grep cmon-cloud
    
        #echo "# killall cmon-cloud"
        #killall cmon-cloud
    
        #echo "# killall -9 cmon-cloud"
        #killall -9 cmon-cloud
    
        echo "# ps aux | grep cmon-cloud"
        ps aux | grep cmon-cloud

        echo "# sudo apt -y --force-yes remove clustercontrol-cloud"
        sudo apt -y --force-yes remove clustercontrol-cloud
    fi


    echo "Exiting test script now."
    return 0
}

function remember_cmon_container()
{
    local container_name="$1"

    if [ -z "$container_name" ]; then
        return 1
    fi

    if [ -n "$CMON_CONTAINER_NAMES" ]; then
        CMON_CONTAINER_NAMES+=" "
    fi

    CMON_CONTAINER_NAMES+="$container_name"
}

function cmon_container_list()
{
    echo $CMON_CONTAINER_NAMES
}

#
# This is the BASH function that executes a functional test. The functional test
# itself should be implemented as a BASH function.
#
function runFunctionalTest ()
{
    local client_log_file="$HOME/s9s.log"
    local test_skipped=""
    local test_start_time
    local test_end_time
    local force=""
    local shouldSkip

    while [ -n "$1" ]; do
        case "$1" in 
            --force)
                # Execute the test even if the whole test script already failed.
                force="true"
                shift 1
                ;;

            *)
                break
                ;;
        esac
    done

    # Counters for numbering the titles/tests
    let t1_counter+=1
    let t2_counter=0

    TEST_NAME=$1
    shift

    # This shows the beginning of the test.
    print_html ""
    print_html "<!-- p42dc 200 $TEST_NAME -->"

    #
    # This is where we call the function that executes the test. Unless
    # we already have some failed tests.
    #
    if [ -z "$force" ]; then
        if  ! isSuccess; then
            shouldSkip="true"
        fi
    fi

    if [ -n "$shouldSkip" ]; then
        print_title "Skipped Test $TEST_NAME"
        if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
            printf "  %-26s: SKIPPED\n" "$TEST_NAME"
        fi

        test_skipped="true"
        test_elapsed_time=0.0
    else
        test_start_time="$(date +"%s.%N")"
        
        #timeout --kill-after=60m 45m $TEST_NAME $*
        $TEST_NAME $*

        test_end_time="$(date +"%s.%N")"
        test_elapsed_time=$(echo "scale=4; $test_end_time - $test_start_time" | bc)
    fi

    # This is when we have only a brief list.
    if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
        if [ -n "$test_skipped" ]; then
            printf "  %-26s: SKIPPED\n" "$TEST_NAME"
        elif ! isSuccess; then
            printf "  %-26s: FAILURE\n" "$TEST_NAME"
        else 
            printf "  %-26s: SUCCESS\n" "$TEST_NAME"
        fi
    fi


    if [ -n "$test_skipped" ]; then
        # Skipped
        print_html "<!-- p42dc 101 $TEST_NAME -->"
    elif ! isSuccess; then
        # FAILURE
        print_html "<!-- p42dc 102 $TEST_NAME -->"
    else 
        # SUCCESS
        print_html "<!-- p42dc 103 $TEST_NAME -->"
    fi

    # This shows how long it took to execute this particular test case.
    print_html "<!-- p42dc 300 $test_elapsed_time -->"
    
    # This marks the end of the test case.
    print_html "<!-- p42dc 201 $TEST_NAME -->"

    if [ -f "$client_log_file" ]; then
        echo -n "" >"$client_log_file"
    fi
}

function check_mysql_account()
{
    local hostname
    local port
    local account_name
    local account_password
    local database_name=""
    local table_name="testtable"
    local create_table
    local insert_into
    local drop_table
    local select
    local query
    local lines
    local retcode
    local tmp
    local log_file=check_mysql_account.log

    while [ -n "$1" ]; do
        case "$1" in
            --hostname)
                hostname="$2"
                shift 2
                ;;

            --port)
                port="$2"
                shift 2
                ;;

            --account-name)
                account_name="$2"
                shift 2
                ;;

            --account-password)
                account_password="$2"
                shift 2
                ;;

            --database-name)
                database_name="$2"
                shift 2
                ;;

            --table-name)
                table_name="$2"
                shift 2
                ;;

            --create-table)
                create_table="yes"
                shift
                ;;

            --insert-into)
                insert_into="yes"
                shift
                ;;

            --select)
                select="true"
                shift
                ;;

            --drop-table)
                drop_table="yes"
                shift
                ;;

            *)
                break
                ;;
        esac
    done
    
    if [ -n "$hostname" ]; then
        success "  o Test host is '$hostname', ok."
    else
        failure "MySQL host name required."
        return 1
    fi

    if [ -n "$database_name" ]; then
        success "  o Test database is '$database_name', ok."
    fi

    if [ -n "$account_name" ]; then
        success "  o Test account is '$account_name', ok."
        tmp=$(s9s account --list --batch --cluster-id=1 "$account_name")
        if [ "$tmp" == "$account_name" ]; then
            success "  o Account $account_name found in the list, ok."
        else
            failure "Account $account_name was not found in the list."
            #failure "tmp = '$tmp'"
            mys9s account --list --long
        fi
    else
        failure "Account name required."
    fi

    cat <<EOF
    mysql --disable-auto-rehash --batch -h$hostname -P$port -u$account_name -p$account_password $database_name -e "SELECT 41+1"

EOF

    lines=$(\
            mysql \
                --disable-auto-rehash \
                --batch \
                -h$hostname \
                -P$port \
                -u$account_name \
                -p$account_password \
                $database_name \
                -e "SELECT 41+1")

    retcode="$?"

    lines=$(echo "$lines" | tail -n +2);
    lines=$(echo $lines);

    if [ "$retcode" -eq 0 ]; then
        success "  o The return code is 0, ok."
    else
        warning "The return code is '$retcode'."
    fi

    clean_lines=$(echo "$lines" | tr -d ' ')
    if [ "$clean_lines" == "42" ]; then
        success "  o The result is '$lines', ok."
    else
        warning "The result for 'select 41 + 1;' is '$lines'."
    fi

}

#
# A function to do various checks on a PostgreSQL database accound.
#
function check_postgresql_account()
{
    local hostname
    local port
    local account_name
    local account_password
    local database_name="template1"
    local table_name="testtable"
    local create_table
    local insert_into
    local drop_table
    local select
    local query
    local lines
    local retcode
    local tmp
    local log_file=check_postgresql_account.log

    while [ -n "$1" ]; do
        case "$1" in
            --hostname)
                hostname="$2"
                shift 2
                ;;

            --port)
                port="$2"
                shift 2
                ;;

            --account-name)
                account_name="$2"
                shift 2
                ;;

            --account-password)
                account_password="$2"
                shift 2
                ;;

            --database-name)
                database_name="$2"
                shift 2
                ;;

            --table-name)
                table_name="$2"
                shift 2
                ;;

            --create-table)
                create_table="yes"
                shift
                ;;

            --insert-into)
                insert_into="yes"
                shift
                ;;

            --select)
                select="true"
                shift
                ;;

            --drop-table)
                drop_table="yes"
                shift
                ;;

            *)
                break
                ;;
        esac
    done
    
    if [ -n "$hostname" ]; then
        success "  o Test host is '$hostname', ok."
    else
        failure "PostgreSQL host name required."
        return 1
    fi
    
    if [ -n "$database_name" ]; then
        success "  o Test database is '$database_name', ok."
    fi
    
    if [ -n "$account_name" ]; then
        success "  o Test account is '$account_name', ok."
        tmp=$(s9s account --list --cluster-id=1 --batch "$account_name")
        if [ "$tmp" == "$account_name" ]; then
            success "  o Account $account_name found in the list, ok."
        else
            failure "Account $account_name was not found in the list."
            #failure "tmp = '$tmp'"
            mys9s account --list --long
        fi
    else
        failure "Account name required."
    fi

    # We do this all the time, not just when it is requested.
    success "  o Will execute SELECT 41 + 1, ok."
    cat <<EOF
psql -t --username="$account_name" --host="$hostname" --port="$port" --dbname="$database_name" --command="select 41 + 1;"
EOF

    lines=$(PGPASSWORD="$account_password" \
        psql \
            -t \
            --username="$account_name" \
            --host="$hostname" \
            --port="$port" \
            --dbname="$database_name" \
            --command="select 41 + 1;")

    retcode="$?"
    #echo "$lines"

    clean_lines=$(echo "$lines" | tail -n 1);
    clean_lines=$(echo $clean_lines | tr -d ' ');
    
    if [ "$retcode" -eq 0 ]; then
        success "  o The return code is 0, ok."
    else
        warning "The return code is '$retcode'."
    fi

    if [ "$clean_lines" == "42" ]; then
        success "  o The result is '$lines', ok."
    else
        warning "The result for 'select 41 + 1;' is '$lines'."
        echo "$lines"
    fi

    #
    # Testing the CREATE TABLE command.
    #
    if [ -n "$create_table" ]; then
        success "  o Testing of CREATE TABLE is requested, ok."
        query="CREATE TABLE $table_name( \
            NAME           CHAR(50) NOT NULL,\
            VALUE          INT      NOT NULL \
        );"

        [ -f "$log_file" ] && rm "$log_file"
        PGPASSWORD="$account_password" \
            psql \
                -t \
                --username="$account_name" \
                --host="$hostname" \
                --port="$port" \
                --dbname="$database_name" \
                --command="$query" >>"$log_file" 2>>"$log_file"

        retcode="$?" 
        if [ "$retcode" -eq 0 ]; then
            success "  o Create table '$database_name.$table_name' is ok."
        else
            failure "Create table failed."
            cat "$log_file"
        fi
    fi

    #
    # 
    #
    if [ -n "$insert_into" ]; then
        success "  o Testing of INSERT INTO is requested, ok."
        query="INSERT INTO $table_name VALUES('ten', 10);"

        [ -f "$log_file" ] && rm "$log_file"
        PGPASSWORD="$account_password" \
            psql \
                -t \
                --username="$account_name" \
                --host="$hostname" \
                --port="$port" \
                --dbname="$database_name" \
                --command="$query" >>"$log_file" 2>>"$log_file"

        retcode="$?" 
        if [ "$retcode" -eq 0 ]; then
            success "  o Insert into '$database_name.$table_name' is ok."
        else
            failure "Insert into failed."
            cat "$log_file"
        fi
    fi
    
    #
    # 
    #
    if [ -n "$select" ]; then
        success "  o Testing of SELECT is requested, ok."
        query="SELECT * FROM $table_name;"

        PGPASSWORD="$account_password" \
            psql \
                -t \
                --username="$account_name" \
                --host="$hostname" \
                --port="$port" \
                --dbname="$database_name" \
                --command="$query" >>"$log_file" 2>>"$log_file"

        retcode="$?" 
        if [ "$retcode" -eq 0 ]; then
            success "  o Select '$database_name.$table_name' is ok."
        else
            failure "Select failed."
            cat "$log_file"
        fi
    fi

    #
    # 
    #
    if [ -n "$drop_table" ]; then
        success "  o Testing of DROP TABLE is requested, ok."
        query="DROP TABLE $table_name;"

        PGPASSWORD="$account_password" \
            psql \
                -t \
                --username="$account_name" \
                --host="$hostname" \
                --port="$port" \
                --dbname="$database_name" \
                --command="$query" >>"$log_file" 2>>"$log_file"

        retcode="$?" 
        if [ "$retcode" -eq 0 ]; then
            success "  o Drop table '$database_name.$table_name' is ok."
        else
            failure "Drop table failed."
            cat "$log_file"
        fi
    fi
   
    #
    # Printing the databases.
    #
    #return 0
    PGPASSWORD="$account_password" \
        psql \
            -P pager=off \
            --username="$account_name" \
            --host="$hostname" \
            --port="$port" \
            --dbname="$database_name" \
            --command="SELECT datname, datacl FROM pg_database;"
}

function check_job_statistics()
{
    local cluster_id=1
    local scheduled=""
    local finished=""
    local running=""
    local check_metatype=""
    local lines
    local tmp
    local n

    while [ -n "$1" ]; do
        case "$1" in 
            --cluster-id)
                cluster_id=$2
                shift 2
                ;;
            
            --scheduled)
                scheduled=$2
                shift 2
                ;;

            --finished)
                finished=$2
                shift 2
                ;;

            --running)
                running=$2
                shift 2
                ;;
            
            --check-metatype)
                check_metatype="true"
                shift
                ;;

            *)
                failure "check_job_statistics(): Unknown option $1."
                break
                ;;
        esac
    done

    if [ -n "$check_metatype" ]; then
        mys9s metatype --list-properties --type=CmonJobStatistics --long

        tmp=$(s9s metatype --list-properties --type=CmonJobStatistics)
        n=0
        for name in $tmp; do
            case "$name" in
                cluster_id|by_state)
                    success "  o Property '$name' recognized, OK."
                    let n+=1
                    ;;

                *)
                    failure "Property $tmp was not recognized."
                    ;;
            esac
        done

        if [ "$n" -lt 2 ]; then
            failure "Some properties were not found."
        else
            success "  o Found $n properties, OK."
        fi
    fi

    mys9s job --list

    cat <<EOF
    s9s cluster --list --cluster-id=$cluster_id --print-json | \\
        jq .cluster.job_statistics
EOF

    s9s cluster --list --cluster-id=1 --print-json | \
        jq .cluster.job_statistics

    lines=$(s9s cluster --list --cluster-id=1 --print-json | \
        jq .cluster.job_statistics)

    if [ -n "$scheduled" ]; then
        tmp=$(echo "$lines" | jq .by_state.SCHEDULED)
        if [ "$tmp" == "$scheduled" ]; then
            success "  o Number of scheduled jobs is $tmp, OK."
        else
            failure "Number of scheduled jobs is $tmp, should be $scheduled."
        fi
    fi
    
    if [ -n "$finished" ]; then
        tmp=$(echo "$lines" | jq .by_state.FINISHED)
        if [ "$tmp" == "$finished" ]; then
            success "  o Number of finished jobs is $tmp, OK."
        else
            failure "Number of finished jobs is $tmp, should be $finished."
        fi
    fi

    if [ -n "$running" ]; then
        tmp=$(echo "$lines" | jq .by_state.RUNNING)
        if [ "$tmp" == "$running" ]; then
            success "  o Number of running jobs is $tmp, OK."
        else
            failure "Number of running jobs is $tmp, should be $running."
        fi
    fi
}

function check_alarm_statistics()
{
    local cluster_id=1
    local should_have_critical=""
    local check_metatype=""
    local lines
    local n
    local tmp

    while [ -n "$1" ]; do
        case "$1" in 
            --cluster-id)
                cluster_id=$2
                shift 2
                ;;

            --should-have_critical)
                should_have_critical="true"
                shift
                ;;

            --check-metatype)
                check_metatype="true"
                shift
                ;;

            *)
                failure "check_alarm_statistics(): Unknown option $1."
                break
                ;;
        esac
    done

    if [ -n "$check_metatype" ]; then
        mys9s metatype --list-properties --type=CmonAlarmStatistics --long

        tmp=$(s9s metatype --list-properties --type=CmonAlarmStatistics)
        n=0
        for name in $tmp; do
            case "$name" in
                cluster_id|created_after|critical|reported_after|warning)
                    success "  o Property '$name' recognized, OK."
                    let n+=1
                    ;;

                *)
                    failure "Property $tmp was not recognized."
                    ;;
            esac
        done

        if [ "$n" -lt 5 ]; then
            failure "Some properties were not found."
        else
            success "  o Found $n properties, OK."
        fi
    fi

    mys9s alarm --stat --cluster-id=$cluster_id --print-json
    lines=$(s9s alarm --stat --cluster-id=$cluster_id --batch --print-json)

    tmp=$(echo "$lines" | jq .alarm_statistics[0].class_name)
    if [ "$tmp" == '"CmonAlarmStatistics"' ]; then
        success "  o Class name is $tmp, ok."
    else
        failure "Class name is $tmp, should be CmonAlarmStatistics."
    fi

    tmp=$(echo "$lines" | jq .alarm_statistics[0].cluster_id)
    if [ "$tmp" == "$cluster_id" ]; then
        success "  o Cluster ID is $tmp, ok."
    else
        failure "Cluster ID is $tmp, should be $cluster_id."
    fi

    tmp=$(echo "$lines" | jq .alarm_statistics[0].critical)
    if [ -n "$should_have_critical" ]; then
        if [ "$tmp" -gt 0 ]; then
            success "  o Has $tmp critical alarms reported, OK."
        else
            failure "Should have critical alarms in the statistics."
        fi
    fi

}

function S9S_LINES_CONTAINS()
{
    local lines
    local retval=0

    lines="$1"
    shift

    while [ -n "$1" ]; do
        if echo "$lines" | grep -q "$1"; then
            success "  o Text '$1' found, OK."
        else
            failure "Line '$1' was not found."
            retval=1
        fi

        shift
    done

    if [ "$retval" -ne 0 ]; then
        echo "Lines:"
        echo "-----8<------8<------8<------8<------8<------8<------8<------"
        echo "$lines"
        echo "-----8<------8<------8<------8<------8<------8<------8<------"
    fi

    return $retval
}

function S9S_FILE_CONTAINS()
{
    local filename
    local lines
    local retval=0

    filename="$1"
    shift

    if [ -n "$filename" ]; then
        success "  o Checking file '$filename', ok"
    else
        failure "S9S_FILE_CONTAINS(): Filename is not provided."
    fi

    if [ -f "$filename" ]; then
        success "  o File '$filename' exists, ok."
        lines=$(cat "$filename");
    else
        failure "File '$filename' was not found."
    fi


    while [ -n "$1" ]; do
        if echo "$lines" | grep -q "$1"; then
            success "  o Text '$1' found, OK."
        else
            failure "Line '$1' was not found."
            retval=1
        fi

        shift
    done

    return $retval
}


trap clean_up_after_test EXIT

