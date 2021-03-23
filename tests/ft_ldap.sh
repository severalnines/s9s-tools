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

  This script will check the basic ldap support. It will configure the LDAP
  support then authenticate with various usernames and passwords to check that
  the authentication works.

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


function testCreateLdapConfig()
{
    local lines
    local cdtPath="/.runtime/LDAP/configuration"

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

    emit_ldap_config \
        --ldap-url "$LDAP_URL" \
        |\
        tee $OPTION_LDAP_CONFIG_FILE | \
        print_ini_file


    mys9s tree \
        --cmon-user=system \
        --password=secret \
        --cat \
        $cdtPath

    check_exit_code_no_job $?

    lines=$(s9s tree --cmon-user=system --password=secret --cat $cdtPath)
    S9S_LINES_CONTAINS "$lines" \
        "enabled" \
        "ldapServerUri" \
        "ldapAdminPassword" \
        "ldapUserSearchRoot" \
        "ldapUsernameAttributes" \
        "ldapGroupId" \
        "cmonGroupName"
    
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

    mys9s user --stat pipas --print-json
    end_verbatim
}


function testLdapChangePasswd()
{
    #
    #
    #
    print_title "Trying to Change the Password of a User"
    cat <<EOF
  Changing the password on the LDAP server is not yet implemented.
EOF

    begin_verbatim
    mys9s user \
        --change-password \
        --cmon-user="system" \
        --password="secret" \
        --new-password="p" \
        "username"     

    retcode=$?
    if [ $retcode -ne 0 ]; then
        success "  o The change password failed, ok."
    else
        failure "Changing the password for an LDAP user should have failed."
    fi

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

    mys9s user --stat pipas --print-json
    
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

function testLdapUser3()
{
    local username="lpere"

    print_title "Logging in with LDAP user $username"
    cat <<EOF | paragraph
  Logging in with a user that is part of an LDAP group and also not in the root
  of the LDAP tree. Checking that the ldapgroup is there and the user is in the
  ldap related group.
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
        lpere

    check_exit_code_no_job $?

    check_user \
        --user-name    "lpere"  \
        --full-name    "Laszlo Pere" \
        --email        "pipas@domain.hu" \
        --cdt-path     "/" \
        --group        "ldapgroup" \
        --dn           "cn=lpere,cn=ldapgroup,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

function testLdapFailures()
{
    local retcode

    print_title "Testing Failed Logins"

    begin_verbatim

    #
    # Invalid/non-existing username.
    #
    mys9s user \
        --list \
        --long \
        --cmon-user="nosuchuser" \
        --password=p

    retcode=$?

    if [ "$retcode" -ne 0 ]; then
        success "  o command failed, ok"
    else
        failure "This command should have failed."
    fi

    #
    # Non existing distinguished name.
    #
    mys9s user \
        --list \
        --long \
        --cmon-user="cn=nosuchuser,dc=homelab,dc=local" \
        --password=p

    retcode=$?

    if [ "$retcode" -ne 0 ]; then
        success "  o command failed, ok"
    else
        failure "This command should have failed."
    fi
    
    #
    # Wrong password.
    #
    mys9s user \
        --list \
        --long \
        --cmon-user="username" \
        --password=wrongpassword

    retcode=$?

    if [ "$retcode" -ne 0 ]; then
        success "  o command failed, ok"
    else
        failure "This command should have failed."
    fi

    end_verbatim
}

#
# Running the requested tests.
#
runFunctionalTest startTests
runFunctionalTest reset_config
runFunctionalTest grant_user

if [ "$1" ]; then
    for testName in $*; do
        runFunctionalTest "$testName"
    done
else
    runFunctionalTest testCmonDbUser
    runFunctionalTest testCreateLdapGroup
    runFunctionalTest testLdapSupport
    runFunctionalTest testCreateLdapConfig
    runFunctionalTest testLdapUser1
    runFunctionalTest testLdapUser2
    runFunctionalTest testLdapUser3
    runFunctionalTest testLdapChangePasswd

    runFunctionalTest testLdapFailures
fi

endTests

