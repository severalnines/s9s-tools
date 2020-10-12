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

enable_ldap_authentication = true

ldap_uri          = "ldap://192.168.0.63:389"
login_dn          = "cn=admin,dc=homelab,dc=local"
login_dn_password = "p"

users_dn          = "dc=homelab,dc=local"
groups_dn         = "dc=homelab,dc=local"

[advanced_settings]
#username_attribute      = "cn, uid, sAMAccountName"
#real_name_attribute     = "displayName"
#email_attribute         = "mail"
#group_name_attribute    = "cn"
static_member_attribute = "memberUid"
#nested_groups           = false
#network_timeout         = 5
#protocol_version        = "3"
#time_limit              = 5

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
    
    ldap_config |\
        sudo tee /etc/cmon-ldap.cnf | \
        print_ini_file

    print_title "Creating LDAP group"
    begin_verbatim
    mys9s group --create ldapgroup
    mys9s group --list --long
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

function testLdapUser()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
    cat <<EOF | paragraph
  This test will check teh LDAP authentication using the distinguished name at
  the login. This is the first login of the user, a CmonDb shadow will be
  created about the user and that shadow will hold some extra information the
  Cmon Controller needs and will also guarantee a unique user ID.
EOF

    begin_verbatim

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

    check_user \
        --user-name    "username"  \
        --cdt-path     "/" \
        --full-name    "firstname lastname" \
        --group        "ldapgroup" \
        --dn           "cn=username,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

function testLdapUserSimple()
{
    local retcode

    print_title "Checking LDAP Authentication with Username"
    cat <<EOF 
  This test checks the LDAP authentication using the simple name. This is not
  the first login, the existing CmonDb shadow will be found and identified.

EOF
    begin_verbatim

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

    end_verbatim

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

function testLdapUserSecond()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
    cat <<EOF
  This test will check teh LDAP authentication using the distinguished name at
  the login. This is not the first time the user logins, so the CmonDb shadow
  should be found. This shadow contains the origin set to LDAP and so LDAP
  authentication should be used.

EOF

    begin_verbatim
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
    end_verbatim
}

function testLdapUserSimpleFirst()
{
    print_title "Checking LDAP Authentication with Username"
    cat <<EOF 
  This test checks the LDAP authentication using the simple name. This is 
  the first login of this user.

EOF

    begin_verbatim
    mys9s user \
        --list \
        --long \
        --cmon-user="pipas1" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="pipas1" \
        --password=p \
        pipas1

    check_exit_code_no_job $?
    
    check_user \
        --user-name    "pipas1"  \
        --cdt-path     "/" \
        --group        "ldapgroup" \
        --dn           "cn=pipas1,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

function testLdapObject()
{
    print_title "Checking LDAP Authentication with Username"
    cat <<EOF 
  This test checks the LDAP authentication using the simple name. This is 
  the first login of this user. On the LDAP server this user has the class
  simpleSecurityObject, so we test a bit different code here.

EOF

    begin_verbatim
    mys9s user \
        --list \
        --long \
        --cmon-user="userid=pipas2,dc=homelab,dc=local" \
        --password=p

    check_exit_code_no_job $?
   
    mys9s user \
        --stat \
        --long \
        --cmon-user="userid=pipas2,dc=homelab,dc=local" \
        --password=p \
        pipas2

    check_exit_code_no_job $?
    
    check_user \
        --user-name    "pipas2"  \
        --cdt-path     "/" \
        --group        "ldapgroup" \
        --dn           "uid=pipas2,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

function testLdapGroup()
{
    local username="cn=lpere,cn=ldapgroup,dc=homelab,dc=local"

    print_title "Checking LDAP Groups"
    cat <<EOF | paragraph
  Logging in with a user that is part of an LDAP group and also not in the root
  of the LDAP tree.
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
        --cmon-user="lpere" \
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
    runFunctionalTest testLdapUserSimpleFirst
    runFunctionalTest testLdapObject
    runFunctionalTest testLdapFailures
    runFunctionalTest testLdapGroup
fi

endTests

