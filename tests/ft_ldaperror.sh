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
source ./include.sh
source ./include_ldap.sh

OPTION_LDAP_CONFIG_FILE="/tmp/cmon-ldap.cnf"

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


function ldap_config_ok()
{

    cat <<EOF
enabled                = true
ldapServerUri          = "$LDAP_URL"
ldapAdminUser          = "cn=admin,dc=homelab,dc=local"
ldapAdminPassword      = "p"

ldapUserSearchRoot     = "dc=homelab,dc=local"
ldapGroupSearchRoot    = "dc=homelab,dc=local"

[ldap_settings]
ldapUsernameAttributes = "cn"
ldapRealnameAttributes = "displayName,cn"
ldapEmailAttributes    = "mail"
ldapMemberAttributes   = "memberUid"

[mapping1]
ldapGroupId            = "ldapgroup"
cmonGroupName          = "ldapgroup"
EOF

    return 0
}

function ldap_config_bad_group()
{

    cat <<EOF
enabled                = true
ldapServerUri          = "$LDAP_URL"
ldapAdminUser          = "cn=admin,dc=homelab,dc=local"
ldapAdminPassword      = "p"

ldapUserSearchRoot     = "dc=homelab,dc=local"
ldapGroupSearchRoot    = "dc=homelab,dc=local"

[ldap_settings]
ldapUsernameAttributes = "cn"
ldapRealnameAttributes = "displayName,cn"
ldapEmailAttributes    = "mail"
ldapMemberAttributes   = "memberUid"

[mapping1]
ldapGroupId            = "nosuchgroup"
cmonGroupName          = "ldapgroup"
EOF

    return 0
}

function testCreateLdapConfigOk()
{
    print_title "Creating the Cmon LDAP Configuration File"
    cat <<EOF
  This test will create and overwrite the '$OPTION_LDAP_CONFIG_FILE', a 
  configuration file that holds the settings of the LDAP settnings for the 
  Cmon Controller.
EOF
    
    begin_verbatim

    if [ -n "$LDAP_URL" ]; then
        success "  o LDAP URL is $LDAP_URL, OK."
    else
        failure "The LDAP_URL variable is empty."
    fi

    ldap_config_ok |\
        tee $OPTION_LDAP_CONFIG_FILE | \
        print_ini_file


    end_verbatim


}

function testCreateLdapConfigBadGroup()
{
    print_title "Creating the Cmon LDAP Configuration File"
    cat <<EOF
  This test will create and overwrite the '$OPTION_LDAP_CONFIG_FILE', a 
  configuration file that holds the settings of the LDAP settnings for the 
  Cmon Controller.
EOF
    
    begin_verbatim

    if [ -n "$LDAP_URL" ]; then
        success "  o LDAP URL is $LDAP_URL, OK."
    else
        failure "The LDAP_URL variable is empty."
    fi

    ldap_config_bad_group |\
        tee $OPTION_LDAP_CONFIG_FILE | \
        print_ini_file


    end_verbatim


}

function testCmonDbUser()
{
    print_title "Testing User with CmonDb Origin"

    begin_verbatim
    mys9s user --stat pipas

    check_user \
        --user-name    "pipas"  \
        --cdt-path     "/" \
        --group        "testgroup" \
        --dn           "-" \
        --origin       "CmonDb"    

    end_verbatim
}

#
# Checking the successful authentication of an LDAP user with the user 
# "username".
#
function testLdapUser1()
{
    local username="username"

    print_title "Checking LDAP Authentication with user '$username'"

    cat <<EOF | paragraph
  This test checks the LDAP authentication using the simple name. The user
  should be able to authenticate.
EOF

    begin_verbatim

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
        username

    check_exit_code_no_job $?

    check_user \
        --user-name    "$username"  \
        --full-name    "firstname lastname" \
        --email        "username@domain.hu" \
        --cdt-path     "/" \
        --group        "ldapgroup" \
        --dn           "cn=username,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

function testLdapUser2()
{
    local username="pipas1"

    print_title "LDAP Authentication with '$username'"
    cat <<EOF | paragraph
  This test checks the LDAP authentication using the simple name. This is 
  the first login of this user.
EOF

    begin_verbatim
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
        pipas1

    check_exit_code_no_job $?
    
    check_user \
        --user-name    "$username"  \
        --cdt-path     "/" \
        --group        "ldapgroup" \
        --dn           "cn=pipas1,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}


#
# Checking the successful authentication of an LDAP user.
#
function testLdapUserFail1()
{
    local username="username"

    print_title "Checking LDAP Authentication with user '$username'"

    cat <<EOF | paragraph
  This test checks the LDAP authentication using the simple name. The user
  should not be able to authenticate, because the Cmon Group is not created in
  advance.
EOF

    begin_verbatim
    #
    # Searching LDAP groups for user 'username'.
    # Considering cn=ldapgroup,dc=homelab,dc=local as group.
    # Group 'ldapgroup' was not found on Cmon.
    # No Cmon group assigned for user 'username'.
    #
    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    if [ $? -eq 0 ]; then
        failure "The user should've failed to authenticate."
    else
        success "  o Command failed, ok."
    fi

    # FIXME: We should check that the user does not exist.
    end_verbatim
}

#
# Checking the successful authentication of an LDAP user.
#
function testLdapUserFail2()
{
    local username="pipas1"

    print_title "Checking LDAP Authentication with user '$username'"

    cat <<EOF | paragraph
  This test checks the LDAP authentication using the simple name. The user
  should not be able to authenticate, because the Cmon Group is not created in
  advance.
EOF

    begin_verbatim
    #
    # Searching LDAP groups for user 'username'.
    # Considering cn=ldapgroup,dc=homelab,dc=local as group.
    # Group 'ldapgroup' was not found on Cmon.
    # No Cmon group assigned for user 'username'.
    #
    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    if [ $? -eq 0 ]; then
        failure "The user should've failed to authenticate."
    else
        success "  o Command failed, ok."
    fi

    # FIXME: We should check that the user does not exist.
    end_verbatim
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
    runFunctionalTest testCreateLdapGroup
    runFunctionalTest testLdapSupport
    
    runFunctionalTest testCreateLdapConfigBadGroup
    runFunctionalTest testLdapUserFail1
    runFunctionalTest testLdapUserFail2
    
    runFunctionalTest testCreateLdapConfigOk
    runFunctionalTest testLdapUser1
    runFunctionalTest testLdapUser2

    #
    # FIXME: Here is a question: what should happen if now we remove the
    # mapping? The user is already created and so the authentication will
    # succeed.
    #
fi

endTests

