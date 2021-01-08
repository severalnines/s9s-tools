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


function ldap_config()
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

function testGetLdapConfig1()
{
    local lines

    print_title "Checking LDAP Config Before Enabling"
    cat <<EOF
  Getting the LDAP configuration through the RPC and checking some values.
EOF

    begin_verbatim
    mys9s controller       \
        --cmon-user=system \
        --password=secret  \
        --print-json       \
        --get-ldap-config 

    check_exit_code_no_job $?

    # s9s controller --get-ldap-config --cmon-user=system --password=secret | jq .ldap_configuration.enabled | cat
    lines=$(s9s controller \
        --get-ldap-config  \
        --cmon-user=system \
        --password=secret)

    check_reply "$lines" \
        ".request_status"                        "Ok" \
        ".ldap_configuration.enabled"            "false" \
        ".ldap_configuration.ldapAdminPassword"  "null" 
        

    end_verbatim
}

#
# A helper function to emit a JSon string that holds the LDAP configuration.
#
function emit_ldap_settings_json()
{
    local enabled="true"

    while [ -n "$1" ]; do
        case "$1" in
            --enabled)
                enabled="true"
                shift
                ;;

            --disabled)
                enabled="false"
                shift
                ;;

            *)
                failure "emit_ldap_settings_json(): Unknown option '$1'."
                return 1
        esac
    done

    if [ "$enabled" == "true" ]; then
    cat <<EOF
  {
    "enabled": $enabled,
    "ldapAdminPassword": "p",
    "ldapAdminUser": "cn=admin,dc=homelab,dc=local",
    "ldapGroupSearchRoot": "dc=homelab,dc=local",
    "ldapServerUri": "ldap://ldap.homelab.local:389",
    "ldapUserSearchRoot": "dc=homelab,dc=local",
    "groupMappings": [ {
        "cmonGroupName": "ldapgroup",
        "ldapGroupId": "ldapgroup",
        "sectionName": "mapping1"
      }  ],
    "ldapSettings": {
      "ldapEmailAttributes": "mail",
      "ldapGroupClassName": null,
      "ldapGroupIdAttributes": null,
      "ldapGroupNameAttribute": null,
      "ldapMemberAttributes": "memberUid",
      "ldapNetworkTimeout": null,
      "ldapProtocolVersion": null,
      "ldapQueryTimeLimit": null,
      "ldapRealnameAttributes": "displayName,cn",
      "ldapUserClassName": null,
      "ldapUsernameAttributes": "cn"
    },
    "security": {
      "caCertFile": null,
      "certFile": null,
      "keyFile": null
    }
  }
EOF
    else
cat <<EOF
  {
    "enabled": $enabled
  }
EOF
    fi
}

# FIXME: return value on non-parseable input is not set.
function testSetLdapConfigEnabled()
{
    print_title "Setting LDAP Configuration"
    cat <<EOF | paragraph
  This test sets the LDAP configuration using the setLdapConfig call. The exit
  code should show that the call succeeded.
EOF
    
    begin_verbatim
    emit_ldap_settings_json |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json

    check_exit_code_no_job $?
    end_verbatim
}

function testSetLdapConfigDisabled()
{
    print_title "Setting LDAP Configuration"
    cat <<EOF | paragraph
  This test will set the LDAP configuration to disabled state. The exit code
  should show that the call succeeded.
EOF

    begin_verbatim
    emit_ldap_settings_json \
        --disabled \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json

    check_exit_code_no_job $?
    end_verbatim
}

function testSetLdapConfigNoAccess()
{
    local exitcode

    print_title "Setting LDAP Configuration with Insufficient Access Rights"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration while the user that does it
  should have insufficient privileges to do so.
EOF

    begin_verbatim
    emit_ldap_settings_json \
        --disabled \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --print-request    \
            --print-json

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigSyntaxError()
{
    local exitcode

    print_title "Setting LDAP Configuration with Syntax Error"
    cat << EOF | paragraph
  This test will try to set the LDAP config, but the standard input is not a
  well-formed JSon string. The exit code should show an error.
EOF

    begin_verbatim
    echo "not a json string" \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testCreateLdapConfig()
{
    local lines
    local cdtPath="/.runtime/LDAP/configuration"

    print_title "Creating the Cmon LDAP Configuration File"

    cat <<EOF
  This test will create and overwrite the '$OPTION_LDAP_CONFIG_FILE', a 
  configuration file that holds the settings of the LDAP settnings for 
  the Cmon Controller.
EOF

    begin_verbatim

    if [ -n "$LDAP_URL" ]; then
        success "  o LDAP URL is $LDAP_URL, OK."
    else
        failure "The LDAP_URL variable is empty."
    fi

    ldap_config |\
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

function testGetLdapConfig2()
{
    local lines

    print_title "Checking LDAP Config After Enabling"
    cat <<EOF
  Getting the LDAP configuration through the RPC and checking some values.
EOF

    begin_verbatim
    mys9s controller       \
        --cmon-user=system \
        --password=secret  \
        --print-json       \
        --get-ldap-config  \
        --print-request    \
        --print-json

    check_exit_code_no_job $?

    # s9s controller --get-ldap-config --cmon-user=system --password=secret | jq .ldap_configuration.enabled | cat
    lines=$(s9s controller \
        --get-ldap-config  \
        --cmon-user=system \
        --password=secret)

    check_reply "$lines" \
        ".request_status"                       "Ok" \
        ".ldap_configuration.enabled"           "true" \
        ".ldap_configuration.ldapAdminPassword" "p" \
        ".ldap_configuration.ldapAdminUser"     "cn=admin,dc=homelab,dc=local" \
        ".ldap_configuration.ldapGroupSearchRoot" "dc=homelab,dc=local" \
        ".ldap_configuration.ldapServerUri"     "ldap://ldap.homelab.local:389" \
        ".ldap_configuration.ldapUserSearchRoot"  "dc=homelab,dc=local" \
        ".ldap_configuration.ldapSettings.ldapMemberAttributes"    "memberUid" \
        ".ldap_configuration.ldapSettings.ldapRealnameAttributes"  "displayName,cn" \
        ".ldap_configuration.ldapSettings.ldapUsernameAttributes"  "cn" \
        ".ldap_configuration.groupMappings[0].cmonGroupName"  "ldapgroup" \
        ".ldap_configuration.groupMappings[0].ldapGroupId"  "ldapgroup"



        

    end_verbatim
}

function testGetLdapConfig3()
{
    local lines

    print_title "Checking LDAP Config After Disabling"
    cat <<EOF | paragraph
  Getting the LDAP configuration through the RPC and checking some values. This
  test assumes the LDAP should be disabled through the setLdapConfig call
  before.
EOF

    begin_verbatim
    mys9s controller       \
        --cmon-user=system \
        --password=secret  \
        --print-json       \
        --get-ldap-config  \
        --print-request    \
        --print-json

    check_exit_code_no_job $?

    # s9s controller --get-ldap-config --cmon-user=system --password=secret | jq .ldap_configuration.enabled | cat
    lines=$(s9s controller \
        --get-ldap-config  \
        --cmon-user=system \
        --password=secret)

    check_reply "$lines" \
        ".request_status"                       "Ok" \
        ".ldap_configuration.enabled"           "false" 

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
    runFunctionalTest testGetLdapConfig1

    runFunctionalTest testCreateLdapGroup

    # This is for reference when debugging.
    #runFunctionalTest testCreateLdapConfig

    # This is what we test.
    runFunctionalTest testSetLdapConfigEnabled

    runFunctionalTest testGetLdapConfig2
    runFunctionalTest testLdapUser1
    
    runFunctionalTest testSetLdapConfigDisabled
    runFunctionalTest testGetLdapConfig3

    runFunctionalTest testSetLdapConfigSyntaxError
    runFunctionalTest testSetLdapConfigNoAccess
fi

runFunctionalTest --force endTests

