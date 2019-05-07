#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
CLUSTER_NAME="${MYBASENAME}_$$"
CLUSTER_ID=""
PIP_CONTAINER_CREATE=$(which "pip-container-create")
CONTAINER_SERVER=""

cd $MYDIR
source include.sh

#
# Prints usage information and exits.
#
function printHelpAndExit()
{
cat << EOF
Usage: $MYNAME [OPTION]... [TESTNAME]
 
  $MYNAME - Testing for basic LDAP support.

  -h, --help       Print this help and exit.
  --verbose        Print more messages.
  --print-json     Print the JSON messages sent and received.
  --log            Print the logs while waiting for the job to be ended.
  --print-commands Do not print unit test info, print the executed commands.
  --reset-config   Remove and re-generate the ~/.s9s directory.
  --server=SERVER  Use the given server to create containers.
  --ldap-url
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


function ldap_config()
{
    cat <<EOF
#
# Cmon LDAP configuration file created by $MYNAME $VERSION.
#

#
# The URI where the Cmon Controller will find the LDAP server as it is specified
# in the ldap.conf(5).
#
ldap_server_uri = "ldap://192.168.42.42:389"

#
# The default base DN to be used when performing LDAP operations. As it is
# specified in the ldap.conf(5).
#
ldap_base_dn    = "dc=homelab,dc=local"

#
# The credentials of the LDAP admin user.
#
ldap_admin_dn   = "cn=admin,dc=homelab,dc=local"
ldap_admin_pwd  = "p"

EOF
}

function ldap_config_ok()
{
    cat <<EOF
#
# Second Cmon LDAP configuration file created by $MYNAME $VERSION.
#
ldap_server_uri = "ldap://192.168.0.167:389"
ldap_base_dn    = "dc=homelab,dc=local"
ldap_admin_dn   = "cn=admin,dc=homelab,dc=local"
ldap_admin_pwd  = "p"

EOF
}

function ldap_config_disabled()
{
    cat <<EOF
#
# Third Cmon LDAP configuration file created by $MYNAME $VERSION.
#
ldap_is_enabled  = false
ldap_server_uri  = "ldap://192.168.0.167:389"
ldap_base_dn     = "dc=homelab,dc=local"
ldap_admin_dn    = "cn=admin,dc=homelab,dc=local"
ldap_admin_pwd   = "p"

EOF
}

function testCreateLdapConfig()
{
    local lines
    local filename="/etc/cmon-ldap.cnf"

    print_title "Creating the Cmon LDAP Configuration File"
    cat <<EOF
  This test will create and overwrite the '/etc/cmon-ldap.cnf', a configuration
  file that holds the settings of the LDAP settnings for the Cmon Controller.
  
  Here is the configuration we created:
EOF
    
    ldap_config | \
        sudo tee $filename  | \
        print_ini_file
   
    # The ft_full has no superuser rights, so we give the config file to this
    # user.
    sudo chown $USER.$USER $filename
    sudo chmod 0600 $filename

    ls -lha $filename

    #
    #
    #
    mys9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/user_manager
    
    #
    #
    #
    mys9s tree \
        --tree \
        --all \
        --cmon-user=system \
        --password=secret \
        /.runtime
    
    check_exit_code_no_job $?
    
    mys9s tree \
        --list \
        --long \
        --recursive \
        --full-path \
        --all \
        --cmon-user=system \
        --password=secret \
        /
    
    check_exit_code_no_job $?
    

    #
    # Listing the LDAP config in the CDT.
    #
    mys9s tree \
        --list \
        --long \
        --cmon-user=system \
        --password=secret \
        /.runtime/LDAP

    check_exit_code_no_job $?
    
    lines=$(mys9s tree \
            --list \
            --long \
            --batch \
            --cmon-user=system \
            --password=secret \
            /.runtime/LDAP)
    
    if echo "$lines" | grep --quiet "120, 4"; then
        success "  o Device numbers are as they should be, ok."
    else
        failure "Device numbers seem not to be ok."
    fi
    
    if echo "$lines" | grep --quiet -- "-rw-------"; then
        success "  o Access rights as they should be, ok."
    else
        failure "Access rights seem not to be ok."
    fi
    
    if echo "$lines" | grep --quiet " system "; then
        success "  o Owner is as they should be, ok."
    else
        failure "File owner seem not to be ok."
    fi
    
    if echo "$lines" | grep --quiet " admins "; then
        success "  o Group is as they should be, ok."
    else
        failure "File group seem not to be ok."
    fi

    #
    #
    #
    mys9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/LDAP/configuration
    
    check_exit_code_no_job $?

    lines=$(s9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/LDAP/configuration)

    if echo "$lines" | grep --quiet "ldap_server_uri"; then
        success "  o CDT '/.runtime/LDAP/configuration' is readable, ok"
    else
        failure "CDT '/.runtime/LDAP/configuration' is not proper."
    fi
}

function testLdapSupport()
{
    print_title "Checking LDAP Support"
    cat <<EOF
  This test checks if the controller has LDAP support.

EOF

    #
    # The controller info file.
    #
    mys9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/controller
    
    check_exit_code_no_job $?

    if s9s tree --cat --cmon-user=system --password=secret \
        /.runtime/controller | \
        grep -q "have_libldap : true"; 
    then
        success "  o The controller has libldap, ok."
    else
        failure "No LDAP support."
    fi

}

function testCmonDbUser()
{
    print_title "Testing User with CmonDb Origin"

    mys9s user --stat pipas

    check_user \
        --user-name    "pipas"  \
        --cdt-path     "/" \
        --group        "testgroup" \
        --dn           "-" \
        --origin       "CmonDb"    
}

function testAuthFail()
{
    local retcode

    print_title "Checking if LDAP Authentication Fails"
    cat <<EOF
  Trying to authenticate with an LDAP user. The authentication should fail 
  now, we are checking that.

EOF

    mys9s user \
        --list \
        --long \
        --cmon-user="cn=username,dc=homelab,dc=local" \
        --password=p

    retcode=$?
    if [ "$retcode" -ne 0 ]; then
        success "  o The command failed, ok."
    else
        failure "The command should have failed."
    fi
}

function testAuthSuccess()
{
    print_title "Checking if LDAP Authentication Succeeds"
    cat <<EOF 
  Checking if the LDAP authentication succeeds. This time the LDAP
  authentication should work.

EOF

    mys9s user \
        --list \
        --long \
        --cmon-user="pipas2" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="pipas2" \
        --password=p \
        pipas2

    check_exit_code_no_job $?
    
    check_user \
        --user-name    "pipas2"  \
        --cdt-path     "/" \
        --group        "LDAPUsers" \
        --dn           "uid=pipas2,dc=homelab,dc=local" \
        --origin       "LDAP"
}

function testLdapGroup()
{
    local username="cn=lpere,cn=ldapgroup,dc=homelab,dc=local"

    print_title "Checking LDAP Groups"
    cat <<EOF 
  Logging in with a user that is part of an LDAP group and also not in the root
  of the LDAP tree.

EOF

    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="$username" \
        --password=p \
        lpere

    check_exit_code_no_job $?

    check_user \
        --user-name    "lpere"  \
        --cdt-path     "/" \
        --group        "users" \
        --dn           "cn=lpere,cn=ldapgroup,dc=homelab,dc=local" \
        --origin       "LDAP"
}

function testLdapConfigOk()
{
    local filename="/etc/cmon-ldap.cnf"
    local mode

    print_title "Creating an LDAP Config that Works"
    cat <<EOF
  This test will write the LDAP config file through a CDT file. Then the file
  is checked.

EOF

    #ls -lha $filename
    ldap_config_ok | \
        s9s tree --save \
        --cmon-user=system \
        --password=secret \
        "/.runtime/LDAP/configuration"

    check_exit_code_no_job $?
    
    if [ -f "$filename" ]; then
        success "  o The file '$filename' exists, ok."
    else
        failure "File '$filename' does not exist."
    fi

    mode=$(ls -lha "$filename" | awk '{print $1}')
    if [ "$mode" == "-rw-------" ]; then
        success "  o The mode is '$mode', ok."
    else
        failure "The mode is '$mode', should be '-rw-------'."
    fi

    #
    #
    #
    #mys9s tree \
    #    --cat \
    #    --cmon-user=system \
    #    --password=secret \
    #    /.runtime/LDAP/configuration
}

function testLdapConfigDisabled()
{
    print_title "Creating an LDAP Config that is Disabled"

    ls -lha /etc/cmon-ldap.cnf
    ldap_config_disabled | s9s tree --save --cmon-user=system --password=secret "/.runtime/LDAP/configuration"
    check_exit_code_no_job $?
    
    #
    #
    #
    mys9s tree \
        --cat \
        --cmon-user=system \
        --password=secret \
        /.runtime/LDAP/configuration
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
    runFunctionalTest testCmonDbUser
    runFunctionalTest testLdapSupport

    runFunctionalTest testCreateLdapConfig
    runFunctionalTest testAuthFail

    runFunctionalTest testLdapConfigOk
    runFunctionalTest testAuthSuccess

    runFunctionalTest testLdapConfigDisabled
    runFunctionalTest testAuthFail
fi

endTests

