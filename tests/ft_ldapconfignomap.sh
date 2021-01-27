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
cmonGroupName          = "myldapgroup"
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
        --print-request    \
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
    local ldap_url="$LDAP_URL"
    local ldapAdminUser="cn=admin,dc=homelab,dc=local"
    local ldapAdminPassword="p"
    local ldapProtocolVersion="null"
    local caCertFile="null"
    local certFile="null"
    local keyFile="null"
    local ldapGroup="ldapgroup"

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

            --ldap-url)
                ldap_url="$2"
                shift 2
                ;;

            --ldap-admin-user)
                ldapAdminUser="$2"
                shift 2
                ;;

            --ldap-admin-password)
                ldapAdminPassword="$2"
                shift 2
                ;;

            --ldap-protocol-version)
                ldapProtocolVersion="$2"
                shift 2
                ;;

            --ca-cert-file)
                caCertFile="$2"
                [ "$caCertFile" != "null" ] && caCertFile="\"$caCertFile\""
                shift 2
                ;;
            
            --cert-file)
                certFile="$2"
                [ "$certFile" != "null" ] && certFile="\"$certFile\""
                shift 2
                ;;
            
            --key-file)
                keyFile="$2"
                [ "$keyFile" != "null" ] && keyFile="\"$keyFile\""
                shift 2
                ;;

            --ldap-group)
                ldapGroup="$2"
                shift 2
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
    "ldapAdminPassword": "$ldapAdminPassword",
    "ldapAdminUser": "$ldapAdminUser",
    "ldapGroupSearchRoot": "dc=homelab,dc=local",
    "ldapServerUri": "$ldap_url",
    "ldapUserSearchRoot": "dc=homelab,dc=local",
    "groupMappings": [ {
        "cmonGroupName": "myldapgroup",
        "ldapGroupId": "$ldapGroup",
        "sectionName": "mapping1"
      }  ],
    "ldapSettings": {
      "ldapEmailAttributes": "mail",
      "ldapGroupClassName": null,
      "ldapGroupIdAttributes": null,
      "ldapGroupNameAttribute": null,
      "ldapMemberAttributes": "memberUid",
      "ldapNetworkTimeout": null,
      "ldapProtocolVersion": $ldapProtocolVersion,
      "ldapQueryTimeLimit": null,
      "ldapRealnameAttributes": "displayName,cn",
      "ldapUserClassName": null,
      "ldapUsernameAttributes": "cn"
    },
    "security": {
      "caCertFile": $caCertFile,
      "certFile": $certFile,
      "keyFile": $keyFile
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
function testSetLdapConfig()
{
    print_title "Setting LDAP Configuration"
    cat <<EOF | paragraph
  This test sets the LDAP configuration using the setLdapConfig call. The exit
  code should show that the call succeeded and this LDAP configuration should
  actually work as it is checked in the next test.
EOF
    
    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json \\
            --ldap-group "ldapgroup" \\
        |  \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json    \
            --ldap-group "ldapgroup" \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json       \
            --color=always

    check_exit_code_no_job $?

    cat $OPTION_LDAP_CONFIG_FILE | print_ini_file
    S9S_FILE_CONTAINS "$OPTION_LDAP_CONFIG_FILE" \
        "^enabled = true" \
        "^ldapServerUri = \"ldap://ldap.homelab.local:389\"" \
        "^ldapAdminUser = \"cn=admin,dc=homelab,dc=local\"" \
        "^ldapAdminPassword = \"p\"" \
        "^ldapUserSearchRoot = \"dc=homelab,dc=local\"" \
        "^ldapGroupSearchRoot = \"dc=homelab,dc=local\"" \
        "^ldapUsernameAttributes = \"cn\"" \
        "^ldapRealnameAttributes = \"displayName,cn\"" \
        "^ldapEmailAttributes = \"mail\"" \
        "^ldapMemberAttributes = \"memberUid\"" \
        "^ldapGroupId   = \"ldapgroup\"" \
        "^cmonGroupName = \"myldapgroup\"" \
        "^ldapUsernameAttributes"

    end_verbatim
}

function testSetLdapConfigWrongLdapGroup()
{
    print_title "Setting LDAP Configuration with Wrong Group"
    cat <<EOF | paragraph
  This test will save an LDAP configuration that has the worng group mapping 
  (an unexisting LDAP group, no existing LDAP group). This intended to ruin
  the configuration so that we can test that users will not be able top log 
  in even if they logged in before.
EOF
    
    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json \\
            --ldap-group "nosuchgroup" \\
        |  \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json    \
            --ldap-group "nosuchgroup" \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json       \
            --color=always

    check_exit_code_no_job $?

    cat $OPTION_LDAP_CONFIG_FILE | print_ini_file
    S9S_FILE_CONTAINS "$OPTION_LDAP_CONFIG_FILE" \
        "^enabled = true" \
        "^ldapServerUri = \"ldap://ldap.homelab.local:389\"" \
        "^ldapAdminUser = \"cn=admin,dc=homelab,dc=local\"" \
        "^ldapAdminPassword = \"p\"" \
        "^ldapUserSearchRoot = \"dc=homelab,dc=local\"" \
        "^ldapGroupSearchRoot = \"dc=homelab,dc=local\"" \
        "^ldapUsernameAttributes = \"cn\"" \
        "^ldapRealnameAttributes = \"displayName,cn\"" \
        "^ldapEmailAttributes = \"mail\"" \
        "^ldapMemberAttributes = \"memberUid\"" \
        "^ldapGroupId   = \"nosuchgroup\"" \
        "^cmonGroupName = \"myldapgroup\"" \
        "^ldapUsernameAttributes"

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
        --group        "myldapgroup" \
        --dn           "cn=username,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

function testLdapUser1Fail()
{
    local username="username"

    print_title "Checking If LDAP Authentication Fails"

    cat <<EOF | paragraph
  This test will check if the user is indeed not able to log in.
EOF

    begin_verbatim

    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    if [ $? -eq 0 ]; then
        failure "The user should not be able to log in."
    else
        success "  o The user can't log in, ok."
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
    runFunctionalTest testGetLdapConfig1

    runFunctionalTest testCreateLdapGroup --group-name myldapgroup


    # This is what we test.
    runFunctionalTest testSetLdapConfig
    runFunctionalTest testLdapUser1

    runFunctionalTest testSetLdapConfigWrongLdapGroup
    runFunctionalTest testLdapUser1Fail
fi

runFunctionalTest --force endTests

