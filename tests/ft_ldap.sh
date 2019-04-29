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
ldap_server_uri = "ldap://192.168.0.167:389"

#
# The default base DN to be used when performing LDAP operations. As it is
# specified in the ldap.conf(5).
#
ldap_base_dn    = "dc=homelab,dc=local"

ldap_admin_dn   = "cn=admin,dc=homelab,dc=local"
ldap_admin_pwd  = "p"
EOF
}

function testCreateLdapConfig()
{
    print_title "Creating the Cmon LDAP Configuration File"
    cat <<EOF
  This test will create and overwrite the '/etc/cmon-ldap.cnf', a configuration
  file that holds the settings of the LDAP settnings for the Cmon Controller.
  
  Here is the configuration we created:
EOF
    
    echo "--------8<--------8<--------8<--------8<--------8<--------8<--------"
    echo -en "$TERM_BOLD"
    ldap_config | sudo tee /etc/cmon-ldap.cnf
    echo -en "$TERM_NORMAL"
    echo "--------8<--------8<--------8<--------8<--------8<--------8<--------"
}

function testLdapSupport()
{
    print_title "Checking LDAP Support"
    cat <<EOF
  This test checks if the controller has LDAP support.

EOF

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
}

function testLdapUser()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
    cat <<EOF
  This test will check teh LDAP authentication using the distinguished name at
  the login. This is the first login of the user, a CmonDb shadow will be
  created about the user and that shadow will hold some extra information the
  Cmon Controller needs and will also guarantee a unique user ID.

EOF

    mys9s user \
        --list \
        --long \
        --cmon-user="cn=username,dc=homelab,dc=local" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="cn=username,dc=homelab,dc=local" \
        --password=p \
        username

    check_exit_code_no_job $?
}

function testLdapUserSimple()
{
    print_title "Checking LDAP Authentication with Username"
    cat <<EOF 
  This test checks the LDAP authentication using the simple name. This is not
  the first login, the existing CmonDb shadow will be found and identified.

EOF

    mys9s user \
        --list \
        --long \
        --cmon-user="username" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="username" \
        --password=p \
        username

    check_exit_code_no_job $?
}

function testLdapUserSecond()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
    cat <<EOF
  This test will check teh LDAP authentication using the distinguished name at
  the login. This is not the first time the user logins, so the CmonDb shadow
  should be found. This shadow contains the origin set to LDAP and so LDAP
  authentication should be used.

EOF

    mys9s user \
        --list \
        --long \
        --cmon-user="cn=username,dc=homelab,dc=local" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="cn=username,dc=homelab,dc=local" \
        --password=p \
        username

    check_exit_code_no_job $?
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
    runFunctionalTest testLdapUser
    runFunctionalTest testLdapUserSimple
    runFunctionalTest testLdapUserSecond
fi

endTests

