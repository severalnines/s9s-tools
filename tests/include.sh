
S9S=$(which s9s)
FAILED="no"
TEST_SUITE_NAME=""
TEST_NAME=""
DONT_PRINT_TEST_MESSAGES=""
PRINT_COMMANDS=""

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

if [ -x ../s9s/s9s ]; then
    S9S="../s9s/s9s"
fi

function color_command()
{
    sed \
        -e "s#\(--.\+\)=\([^\\]*\)[\ ]#\x1b\[0;33m\1\x1b\[0;39m=\"\x1b\[1;35m\2\x1b\[0;39m\" #g" \
        -e "s#\(--[^\\\ ]\+\)#\x1b\[0;33m\1\x1b\[0;39m#g"
}

function prompt_string
{
    local dirname=$(basename $PWD)

    echo "$USER@$HOSTNAME:$dirname\$"
}

function mysleep()
{
    local prompt=$(prompt_string)
        
    echo -ne "$prompt ${XTERM_COLOR_YELLOW}sleep${TERM_NORMAL} "
    echo -ne " ${XTERM_COLOR_ORANGE}$@${TERM_NORMAL}"
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

    prompt="$username@$hostname:$dirname\$"

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

function mys9s_singleline()
{
    local prompt=$(prompt_string)
    local nth=0

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

    $S9S --color=always "$@"
}

function mys9s_multiline()
{
    local prompt=$(prompt_string)
    local nth=0

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

    $S9S --color=always "$@"
}

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

t1_counter="0"
t2_counter="0"

#
# Prints a title line that looks nice on the terminal and also on the web.
#
function print_title()
{
    local number

    let t1_counter+=1
    let t2_counter=0

    number="${t1_counter}"
    if [ -t 1 ]; then
        echo ""
        echo -e "$TERM_COLOR_TITLE$number $*\033[0;39m"
        echo -e "\033[1m\
-------------------------------------------------------------------------------\
-\033[0;39m"
    else
        echo "</pre>"
        echo ""
        echo "<h3>$number $*</h3>"
        echo "<pre>"
    fi
}

function print_subtitle()
{
    local number
    let t2_counter+=1

    number="${t1_counter}.${t2_counter}"
    if [ -t 1 ]; then
        echo ""
        echo -e "$TERM_COLOR_TITLE$number $*\033[0;39m"
    else
        echo "</pre>"
        echo ""
        echo "<h4>$number $*</h4>"
        echo "<pre>"
    fi
}

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

    TEST_SUITE_NAME=$(basename $0 .sh)
    pip-host-control --status="Running '$TEST_SUITE_NAME'."

    #
    # Printing some info
    #
    echo "Starting test $TEST_SUITE_NAME"
    if [ -n "$COMMAND_LINE_OPTIONS" ]; then
        echo "  Command line: $COMMAND_LINE_OPTIONS"
    fi

    echo " TEST_SUITE_NAME: $TEST_SUITE_NAME"
    echo "        hostname: $(hostname)"
    echo "           MYDIR: $MYDIR"
    echo "         VERSION: $VERSION"
    echo "            USER: $USER"
    echo "         OPTIONS: $OPTIONS"

    #
    # Doing some checks
    #
    echo -n "Checking if jq is installed..."
    if [ -z "$(which jq)" ]; then
        echo "[INSTALLING]"
        sudo apt-get install -y --force-yes jq
    else
        echo "[OK]"
    fi

    echo -n "Checking if s9s is installed..."
    if [ -z "$S9S" ]; then
        echo " [FAILED]"
        exit 7
    else 
        echo " [OK]"
    fi

    echo -n "Searching for pip-container-create..."
    if [ -z $(which pip-container-create) ]; then
        echo " [FAILED]"
        exit 7
    else 
        echo " [OK]"
    fi

    if [ -z "$(which highlight)" ]; then
        echo "Installing the highlight package."
        sudo apt -y --force-yes install highlight
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
        echo "Installing 'pipscripts'."
        pushd "$MYDIR/../pipscripts"
        sudo make install 
        popd
    fi

    if [ -f "$HOME/.s9s/s9s.state" ]; then
        echo "Removing '$HOME/.s9s/s9s.state'."
        rm -f $HOME/.s9s/s9s.state
    fi

    if [ -f "/etc/cmon-ldap.cnf" ]; then
        echo "Removing '/etc/cmon-ldap.cnf'..."
        sudo rm -f "/etc/cmon-ldap.cnf"
    fi

#    if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
        echo ""
        echo "***********************"
        echo "* $TEST_SUITE_NAME"
        echo "***********************"
#    fi
}

#
# This function should be called when the function tests are executed. It prints
# a message about the results and exits with the exit code that is true if the
# tests are passed and false if at least one test is failed.
#
function endTests ()
{
    if isSuccess; then
        if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
            echo "SUCCESS: $(basename $0 .sh)"
        else
            print_title "Report"
            echo -en "${XTERM_COLOR_GREEN}"
            echo -n  "Test $(basename $0) is successful."
            echo -en "${TERM_NORMAL}"
            echo ""
        fi

        printf "  Performed: %'4d check(s)\n" "$NUMBER_OF_PERFORMED_CHECKS"
        printf "    Success: %'4d check(s)\n" "$NUMBER_OF_SUCCESS_CHECKS"
        printf "     Failed: %'4d check(s)\n" "$NUMBER_OF_FAILED_CHECKS"

        pip-host-control --status="Passed '$TEST_SUITE_NAME'."
        
        exit 0
    else
        if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
            echo "FAILURE: $(basename $0 .sh)"
        else
            print_title "Report"
            echo -en "${XTERM_COLOR_RED}"
            echo -n  "Test $(basename $0) has failed."
            echo -en "${TERM_NORMAL}"
            echo ""
        fi
    
        pip-host-control --status="Failed '$TEST_SUITE_NAME'."
        exit 1
    fi
}

#
# This is the BASH function that executes a functional test. The functional test
# itself should be implemented as a BASH function.
#
function runFunctionalTest ()
{
    TEST_NAME=$1

    if ! isSuccess; then
        if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
            printf "  %-26s: SKIPPED\n" "$TEST_NAME"
        fi

        return 1
    else
        $TEST_NAME $*
    fi

    if [ -z "$DONT_PRINT_TEST_MESSAGES" ]; then
        if ! isSuccess; then
            printf "  %-26s: FAILURE\n" "$TEST_NAME"
        else 
            printf "  %-26s: SUCCESS\n" "$TEST_NAME"
        fi
    fi
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
        echo "FAILURE: $1"
    fi

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

        if [ "$do_not_exit" ]; then
            return 1
        else
            exit $exitCode
        fi
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
    fi

    return 0
}

function check_job()
{
    local job_id
    local state
    local required_state

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

            *)
                break
                ;;
        esac
    done

    state=$(s9s job --list --job-id=$job_id --batch | awk '{print $3}')
    if [ "$state" == "$required_state" ]; then
        success "  o Job $job_id is $state, ok"
    else
        failure "Job $job_id state is $state, not $required_state."
    fi
}

#
#
#
function check_log()
{
    local log_file="/tmp/cmon.log"
    local line

    if [ -n "$log_file" ]; then
        success "  o Will check log file '$log_file', ok."

        if [ -f "$log_file" ]; then
            success "  o Log file '$log_file' exists, ok."
        else
            failure "Log file '$log_file' does not exist."
        fi

        for line in "$@"; do
            if grep --quiet "$line" $log_file; then
                success "  o Expression found: '$line'"
            else
                failure "Expression not found: '$line'"
            fi
        done
    fi
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

    group=$(s9s container --list --container-format="%G\n" "$container_name")
    if [ -n "$expected_group" -a "$group" != "$expected_group" ]; then
        failure "  o The group should be '$expected_group', not '$group'."
    else
        success "  o The group of the container is $group, ok"
    fi

    path=$(s9s container --list --container-format="%p\n" "$container_name")
    if [ -n "$expected_path" -a "$path" != "$expected_path" ]; then
        failure "  o The path should be '$expected_path', not '$path'."
    else
        success "  o The path of the container is $path, ok"
    fi
    
    acl=$(s9s container --list --container-format="%l\n" "$container_name")
    if [ -n "$expected_acl" -a "$acl" != "$expected_acl" ]; then
        failure "  o The acl should be '$expected_acl', not '$acl'."
    else
        success "  o The acl of the container is '$acl', ok"
    fi

    cloud=$(s9s container --list --container-format="%c\n" "$container_name")
    if [ -n "$expected_cloud" -a "$cloud" != "$expected_cloud" ]; then
        failure "  o The cloud should be '$expected_cloud', not '$cloud'."
    else
        success "  o The cloud of the container is $cloud, ok"
    fi

    state=$(s9s container --list --container-format="%S\n" "$container_name")
    if [ -n "$expected_state" -a "$state" != "$expected_state" ]; then
        failure "  o The state should be '$expected_state', not '$state'."
    else
        success "  o The state of the container is $state, ok"
    fi

    parent=$(s9s container --list --container-format="%P\n" "$container_name")
    if [ -n "$expected_parent" -a "$parent" != "$expected_parent" ]; then
        failure "  o The parent should be '$expected_parent', not '$parent'."
    else
        success "  o The parent of the container is $parent, ok"
    fi

    prot=$(s9s container --list --container-format="%T\n" "$container_name")
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

function maxscale_node_name()
{
    s9s node --list --long --batch |\
        grep '^x' | \
        awk '{print $5 }'
}

function proxysql_node_name()
{
    s9s node --list --long --batch |\
        grep '^y' | \
        awk '{print $5 }'
}

function galera_node_name()
{
    s9s node --list --long --batch | \
        grep '^g' | \
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
    node_ips=$(s9s node --list --long | grep "$filter" | awk '{ print $5 }')
    for node_ip in $node_ips; do
        print_title "Checking Node $node_ip"

        container_id=$(node_container_id "$node_ip")
        echo "      node_ip: $node_ip"
        echo " container_id: $container_id"
            
        if [ -z "$container_id" ]; then
            failure "The container ID was not found."
        fi

        if [ "$container_id" == "-" ]; then
            failure "The container ID is '-'."
        fi
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
                failure "Waited ${waited}s to reach state $expectedState."
                return 1
            fi
        fi

        if [ "$stayed" -gt 10 ]; then
            success "  o Cluster is in $expectedState, ok."
            return 0
        fi

        let waited+=1
        sleep 1
    done

    return 2
}

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

    serverName="$1"
    owner="$2"
    keyfile="$3"

    if [ "$keyfile" ]; then
        keyOption="-i $keyfile"
    fi

    if [ "$option_current_user" ]; then
        isOk=$(\
            ssh -o ConnectTimeout=1 \
                -o UserKnownHostsFile=/dev/null \
                -o StrictHostKeyChecking=no \
                -o LogLevel=quiet \
                $keyOption \
                "$owner@$serverName" \
                2>/dev/null -- echo OK)
    else
        isOk=$(sudo -u $owner -- \
            ssh -o ConnectTimeout=1 \
                -o UserKnownHostsFile=/dev/null \
                -o StrictHostKeyChecking=no \
                -o LogLevel=quiet \
                $keyOption \
                "$serverName" \
                2>/dev/null -- echo OK)
    fi

    if [ "$isOk" == "OK" ]; then
        return 0
    fi

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

            # 120 x 5 = 10 minutes
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
    local container_name
    local args

    args=$(\
    getopt -o h \
        -l "help,autodestroy,template:" \
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
                shift 2
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
        printError "The container server is not set."
        return 1
    fi

    container_name=$1

    echo -n "Creating container..." >&2
    ip=$(pip-container-create \
        $template_option \
        $verbose_option \
        --server=$CONTAINER_SERVER $container_name)

    retval=$?
    if [ "$retval" -ne 0 ]; then
        printError "pip-container-create returned ${retval}."
        tail $HOME/pip-container-create.log >&2
    fi

    printVerbose "Created '$ip'."
   
    #
    # Waiting until the server is up and accepts SSH connections.
    #
    wait_for_server_ssh "$ip" "$USER"
    retval=$?
    if [ "$retval" -ne 0 ]; then
        echo -e " $XTERM_COLOR_RED[FAILURE]$TERM_NORMAL" >&2
        echo "Could not reach created server at ip '$ip'." >&2
    else
        echo -e " $XTERM_COLOR_GREEN[SUCCESS]$TERM_NORMAL" >&2
    fi

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
    cat <<EOF
#
# This configuration file was created by ${MYNAME} version ${VERSION}.
#
[global]
controller = https://localhost:9556

[log]
brief_job_log_format = "%36B:%-5L: %-7S %M\n"
brief_log_format     = "%C %36B:%-5L: %-8S %M\n"
EOF
}

function reset_config()
{
    local config_dir="$HOME/.s9s"
    local config_file="$config_dir/s9s.conf"
    local do_not_create

    while [ "$1" ]; do
        case "$1" in 
            --do-not-create)
                shift
                do_not_create="true"
                ;;

            *)
                break
        esac
    done

    if [ -z "$OPTION_RESET_CONFIG" ]; then
        return 0
    fi
   
    if [ -z "$do_not_create" ]; then
        print_title "Overwriting s9s Configuration"
    else
        print_title "Deleting s9s Configuration"
    fi

    if [ -d "$config_dir" ]; then
        rm -rf "$config_dir"
    fi

    if [ -z "$do_not_create" ]; then
        if [ ! -d "$config_dir" ]; then
            mkdir "$config_dir"
        fi

        emit_s9s_configuration_file >$config_file

        # This goes to the standard output.
        emit_s9s_configuration_file | print_ini_file 
    fi

    # FIXME: This should not be here:
    sudo rm -f $HOME/pip-container-create.log 2>/dev/null
}

#
# $1: the name of the cluster
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

    print_title "Creating the First User"
    first=$(getent passwd $USER | cut -d ':' -f 5 | cut -d ',' -f 1 | cut -d ' ' -f 1)
    last=$(getent passwd $USER | cut -d ':' -f 5 | cut -d ',' -f 1 | cut -d ' ' -f 2)

    mys9s user \
        --create \
        --group="testgroup" \
        --create-group \
        --generate-key \
        --controller="https://localhost:9556" \
        --new-password="p" \
        --email-address="laszlo@severalnines.com" \
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

    #
    # Adding the user's default SSH public key. This will come handy when we
    # create a container because this way the user will be able to log in with
    # the SSH key without password.
    #
    mys9s user \
        --add-key \
        --public-key-file="/home/$USER/.ssh/id_rsa.pub" \
        --public-key-name="The_SSH_key"

    #mys9s user --list-keys
    #mys9s server --list --long
    #mys9s server --unregister --servers=cmon-cloud://localhost
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
        tmp=$(s9s node --list --node-format "%A\n" "$hostname")

        if [ "$tmp" == "$ipaddress" ]; then
            success "  o The IP of the node is $tmp, ok."
        else
            failure "The IP of the node should not be '$tmp'."
        fi
    fi
    
    if [ -n "$port" ]; then
        tmp=$(s9s node --list --node-format "%P\n" "$hostname")

        if [ "$tmp" == "$port" ]; then
            success "  o The port of the node is $tmp, ok."
        else
            failure "The port of the node should not be '$tmp'."
        fi
    fi

    if [ -n "$owner" ]; then
        tmp=$(s9s node --list --node-format "%O\n" "$hostname")

        if [ "$tmp" == "$owner" ]; then
            success "  o The owner of the node is $tmp, ok."
        else
            failure "The owner of the node should not be '$tmp'."
        fi
    fi

    if [ -n "$group" ]; then
        tmp=$(s9s node --list --node-format "%G\n" "$hostname")

        if [ "$tmp" == "$group" ]; then
            success "  o The group of the node is $tmp, ok."
        else
            failure "The group of the node should not be '$tmp'."
        fi
    fi

    if [ -n "$cdt_path" ]; then
        tmp=$(s9s node --list --node-format "%h\n" "$hostname")

        if [ "$tmp" == "$cdt_path" ]; then
            success "  o The CDT path of the node is $tmp, ok."
        else
            failure "The CDT path of the node should not be '$tmp'."
        fi

    fi

    if [ -n "$status" ]; then
        tmp=$(s9s node --list --node-format "%S\n" "$hostname")

        if [ "$tmp" == "$status" ]; then
            success "  o The status of the node is $tmp, ok."
        else
            failure "The status of the node should not be '$tmp'."
        fi
    fi
    
    if [ -n "$config" ]; then
        tmp=$(s9s node --list --node-format "%C\n" "$hostname")

        if [ "$tmp" == "$config" ]; then
            success "  o The config file of the node is $tmp, ok."
        else
            failure "The config file of the node should not be '$tmp'."
        fi
    fi
    
    if [ -n "$no_maintenance" ]; then
        tmp=$(s9s node --list --node-format "%a\n" "$hostname")

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
        tmp=$(s9s cluster --list --cluster-format "%C\n" "$cluster_name")

        if [ "$tmp" == "$config_file" ]; then
            success "  o The config file of the cluster is $tmp, ok."
        else
            failure "The config file of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$log_file" ]; then
        tmp=$(s9s cluster --list --cluster-format "%L\n" "$cluster_name")

        if [ "$tmp" == "$log_file" ]; then
            success "  o The log file of the cluster is $tmp, ok."
        else
            failure "The log file of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$owner" ]; then
        tmp=$(s9s cluster --list --cluster-format "%O\n" "$cluster_name")

        if [ "$tmp" == "$owner" ]; then
            success "  o The owner of the cluster is $tmp, ok."
        else
            failure "The owner of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$group" ]; then
        tmp=$(s9s cluster --list --cluster-format "%G\n" "$cluster_name")

        if [ "$tmp" == "$group" ]; then
            success "  o The group of the cluster is $tmp, ok."
        else
            failure "The group of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$cdt_path" ]; then
        tmp=$(s9s cluster --list --cluster-format "%P\n" "$cluster_name")

        if [ "$tmp" == "$cdt_path" ]; then
            success "  o The CDT path of the cluster is $tmp, ok."
        else
            failure "The CDT path of the cluster should not be '$tmp'."
        fi
    fi
    
    if [ -n "$cluster_type" ]; then
        tmp=$(s9s cluster --list --cluster-format "%T\n" "$cluster_name")

        if [ "$tmp" == "$cluster_type" ]; then
            success "  o The type of the cluster is $tmp, ok."
        else
            failure "The type of the cluster should not be '$tmp'."
        fi
    fi

    if [ -n "$cluster_state" ]; then
        tmp=$(s9s cluster --list --cluster-format "%S\n" "$cluster_name")

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

    state=$(s9s replication --list --link-format="%s\n" --slave=$slave)
    if [ "$state" == "$expected_state" ]; then
        success "  o Replication state on $slave is $state, ok."
    else
        failure "Replication state is '$state', should be '$expected_state'."
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
        tmp=$(s9s user --whoami --cmon-user="$user_name" --password="$password")
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
        tmp=$(s9s user --whoami --cmon-user="$user_name")
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
        tmp=$(s9s user --list --user-format="%F\n" "$user_name" | tr ' ' '_')
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
        tmp=$(s9s user --list --user-format="%d\n" "$user_name")
        if [ "$tmp" != "$option_dn" ]; then
            failure "Distinguished name should be name $option_dn, not $tmp."
        else
            success "  o the distinguished name is '$tmp', ok"
        fi
    fi

    if [ -n "$option_origin" ]; then
        tmp=$(s9s user --list --user-format="%o\n" "$user_name")
        if [ "$tmp" != "$option_origin" ]; then
            failure "The origin should be '$option_origin' and not '$tmp'."
        else
            success "  o the origin is '$tmp', ok"
        fi
    fi
    
    if [ -n "$option_cdt_path" ]; then
        tmp=$(s9s user --list --user-format="%P\n" "$user_name")
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

    print_title "Checking Server $container_server"

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
            return 1
        fi
    elif [ -z "$class" ]; then
        failure "Server $container_server has empty class name."
        return 1
    fi

    #
    # Checking the state runtime information.
    #
    echo ""
    file="/$container_server/.runtime/state"
    n_names_found=0
    #mys9s tree --cat $file
    
    IFS=$'\n'
    for line in $(s9s tree --cat $file)
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
    for line in $(s9s tree --cat --cmon-user=system --password=secret $file)
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
    #mys9s server --list-regions $cloud_option

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
    print_title "Preparing to Exit"
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
    if [ "$OPTION_LEAVE_NODES" ]; then
        print_title "Leaving the Containers"
        echo "The --leave-nodes option was provided, not destroying the "
        echo "containers."
        echo "     server : $CONTAINER_SERVER"
        echo " containers : $all_created_ip"
    elif [ "$OPTION_INSTALL" ]; then
        print_title "Leaving the Containers"
        echo "The --install option was provided, not destroying the "
        echo "containers."
        echo "     server : $CONTAINER_SERVER"
        echo " containers : $all_created_ip"
    elif [ "$all_created_ip" ]; then
        print_title "Destroying the Containers"
        echo "     server : $CONTAINER_SERVER"
        echo " containers : $all_created_ip"

        pip-container-destroy \
            --server=$CONTAINER_SERVER \
            "$all_created_ip" \
            >/dev/null 2>/dev/null
        
        echo "    retcode : $?"
    fi

    # Destroying the container list file.
    if [ -f "$container_list_file" ]; then
        rm -f "$container_list_file"
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

trap clean_up_after_test EXIT

