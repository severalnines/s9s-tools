#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
MYHOSTNAME="$(hostname)"
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"

CONTAINER_SERVER="$MYHOSTNAME"
CONTAINER_IP=""

CLUSTER_NAME="${MYBASENAME}_$$"
LAST_CONTAINER_NAME=""
N_CONTAINERS=0

cd $MYDIR
source ./include.sh
source ./include_lxc.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]

 $MYNAME - Test script that installs cmon-cloud on various distros.

 -h, --help       Print this help and exit.
 --verbose        Print more messages.
 --print-json     Print the JSON messages sent and received.
 --log            Print the logs while waiting for the job to be ended.
 --print-commands Do not print unit test info, print the executed commands.
 --install        Just install the server and the cluster and exit.
 --reset-config   Remove and re-generate the ~/.s9s directory.
 --server=SERVER  Use the given server to create containers.

EOF
    exit 1
}

ARGS=$(\
    getopt -o h \
        -l "help,verbose,print-json,log,print-commands,install,reset-config,\
server:" \
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

        --install)
            shift
            OPTION_INSTALL="--install"
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

if [ -z "$CONTAINER_SERVER" ]; then
    printError "No container server specified."
    printError "Use the --server command line option to set the server."
    exit 6
fi

# ubuntu_artful ubuntu_bionic ubuntu_trusty ubuntu_xenial ubuntu_zesty debian_buster debian_jessie debian_sid debian_stretch debian_wheezy alpine_3.4 alpine_3.5 alpine_3.6 alpine_3.7 alpine_edge centos_6 centos_7 fedora_25 fedora_26 fedora_27 opensuse_42.2 opensuse_42.3 oracle_6 oracle_7 plamo_5.x plamo_6.x archlinux_current gentoo_current
function installCmonCloud()
{
    local container_name
    local container_ip
    local container_image="gentoo_current"
    local this_failed=""

    #
    # Processing options and arguments of the function.
    #
    while [ -n "$1" ]; do
        case "$1" in
            --image)
                shift
                container_image="$1"
                shift
                ;;

            *)
                break;
        esac
    done

    #
    # Installing.
    #
    print_title "Installing Cmon-cloud on $container_image"

    begin_verbatim

    container_name=$(printf "${MYBASENAME}_${image}_%02d_$$" $N_CONTAINERS)
    let N_CONTAINERS+=1

    mys9s container \
        --create \
        --image="$container_image" \
        $LOG_OPTION \
        "$container_name"

    if ! check_exit_code --do-not-exit $?; then
        this_failed="true"
    fi
    
    container_ip=$(get_container_ip "$container_name")
    if [ -z "$container_ip" ]; then
        failure "The container '$container_name' was not created or got no IP."
        s9s container --list --long
        end_verbatim
        return 1
    fi

    mys9s server \
        --create \
        --servers="cmon-cloud://$container_ip" \
        $LOG_OPTION

    if ! check_exit_code --do-not-exit $?; then
        this_failed="true"
        #return 1
    fi

    mys9s server --list --batch --long "$container_ip"
    mys9s server --stat "$container_ip"
    
    if s9s server --stat "$container_ip" | grep --quiet "CmonHostOffLine"; then
        failure "The server is off-line."
        this_failed="true"
        end_verbatim
        return 1
    else
        success "The server on $image is on-line."
    fi

    end_verbatim

    #
    # Cleaning up after the test.
    #
    print_title "Unregistering Cmon-cloud Server on $image"
    begin_verbatim

    mys9s server \
        --unregister \
        --servers="cmon-cloud://$container_ip"
    
    if ! check_exit_code_no_job $?; then
        this_failed="true"
    fi

    mys9s container \
        --delete \
        $LOG_OPTION \
        "$container_name"
    
    check_exit_code $?

    if [ "$this_failed" ]; then
        failure "Installing cmon-cloud on $image failed."
    else 
        success "Installing cmon-cloud on $image worked."
    fi

    end_verbatim
    return 0
}

function testInstall()
{
    local tmp_file=$(mktemp)
    local line
    local image
    local counter=0
    local images="ubuntu_xenial \
      debian_buster debian_stretch debian_jessie \
      centos_7 oracle_7 ubuntu_trusty"

    # 
    # these has issues with /etc/sudoers: fedora_26 fedora_27
    #images+=" fedora_26 fedora_27"

    # these has issues with ssh: opensuse_42.3 gentoo_current
    #images+=" opensuse_42.3 gentoo_current"

    # these has other issues: ubuntu_trusty debian_wheezy centos_6 oracle_6 debian_sid
    #images+=" debian_wheezy centos_6 oracle_6 debian_sid"
    
    for image in $images; do
        installCmonCloud --image "$image"
        if [ $? -ne 0 ]; then
            stat="$XTERM_COLOR_RED[FAILURE]$TERM_NORMAL"
        else
            stat="$XTERM_COLOR_GREEN[SUCCESS]$TERM_NORMAL"
        fi
            
        line=$(printf "%03d %-20s $stat\n" "$counter" "$image")

        echo "$line" >>"$tmp_file"
        let counter+=1

        #if [ "$counter" -gt 10 ]; then
        #    break
        #fi
    done

    print_title "Summary of Tests"
    begin_verbatim
    cat "$tmp_file"
    rm -f "$tmp_file"
    end_verbatim
}

#
# Running the requested tests.
#
startTests
reset_config
grant_user

if [ "$OPTION_INSTALL" ]; then
    runFunctionalTest registerServer
elif [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest registerServer
    runFunctionalTest testInstall
fi

endTests
