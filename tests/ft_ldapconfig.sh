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
    local ldapUserSearchRoot="dc=homelab,dc=local"

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

            --ldap-user-search-root)
                ldapUserSearchRoot="$2"
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
    "ldapUserSearchRoot": "$ldapUserSearchRoot",
    "groupMappings": [ {
        "cmonGroupName": "myldapgroup",
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
function testSetLdapConfigEnabled()
{
    print_title "Setting LDAP Configuration"
    cat <<EOF | paragraph
  This test sets the LDAP configuration using the setLdapConfig call. The exit
  code should show that the call succeeded and this LDAP configuration should
  actually work as it is checked in the next test.
EOF
    
    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json |  \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json |  \
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

function testSetLdapConfigDisabled()
{
    print_title "Setting LDAP Configuration"
    cat <<EOF | paragraph
  This test will set the LDAP configuration to disabled state. The exit code
  should show that the call succeeded.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json \\
        --disabled \\
        |  \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always 
EOF

    emit_ldap_settings_json \
        --disabled \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json       \
            --color=always 

    check_exit_code_no_job $?
    end_verbatim
}

function testSetLdapConfigWrongUserSearch()
{
    print_title "Setting LDAP Configuration with Wrong User Search Root"
    cat <<EOF | paragraph
    ...
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json \\
        --ldap-user-search-root "dc=wrong,dc=local" \\
        |  \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always 
EOF

    emit_ldap_settings_json \
        --ldap-user-search-root "dc=wrong,dc=local" \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json       \
            --color=always 

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
    cat <<EOF
    # emit_ldap_settings_json \\
        --disabled \\
        |  \\
        s9s controller         \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --disabled \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --print-request    \
            --print-json       \
            --color=always

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigEmptyUrl()
{
    local exitcode

    print_title "Setting LDAP Configuration with Empty URL"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration with the LDAP URL set to an
  empty string. The call should fail.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json  \\
        --enabled              \\
        --ldap-url ""          \\
        |                      \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --enabled \
        --ldap-url "" \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --cmon-user=system \
            --password=secret  \
            --print-request    \
            --print-json       \
            --color=always

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigWrongUrl()
{
    local exitcode

    print_title "Setting LDAP Configuration with Empty URL"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration with the LDAP URL set to an
  empty string. The call should fail.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json  \\
        --enabled              \\
        --ldap-url "ldap://wrong-url.hu" \\
        |                      \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --enabled \
        --ldap-url "ldap://wrong-url.hu" \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --cmon-user=system \
            --password=secret  \
            --print-request    \
            --print-json       \
            --color=always

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigWrongProtocol()
{
    local exitcode

    print_title "Setting LDAP Configuration with Empty URL"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration with the wrong LDAP protocol
  version.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json  \\
        --enabled              \\
        --ldap-protocol-version "2" \\
        |                      \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --enabled \
        --ldap-protocol-version "2" \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --cmon-user=system \
            --password=secret  \
            --print-request    \
            --print-json       \
            --color=always

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigWrongPassword()
{
    local exitcode

    print_title "Setting LDAP Configuration with Wrong Admin Password"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration with the wrong LDAP admin 
  password.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json  \\
        --enabled              \\
        --ldap-admin-password "2" \\
        |                      \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --enabled \
        --ldap-admin-password "2" \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --cmon-user=system \
            --password=secret  \
            --print-request    \
            --print-json       \
            --color=always

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigWrongAdmin()
{
    local exitcode

    print_title "Setting LDAP Configuration with Wrong Admin Username"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration with the wrong LDAP admin 
  username.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json  \\
        --enabled              \\
        --ldap-admin-user "cn=invalid,dc=homelab,dc=local" \\
        |                      \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --enabled \
        --ldap-admin-user "cn=invalid,dc=homelab,dc=local" \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --cmon-user=system \
            --password=secret  \
            --print-request    \
            --print-json       \
            --color=always

    exitcode=$?
    if [ "$exitcode" -eq 0 ]; then
        failure "The exit code should not be 0."
    else
        success "  o The exit code is $exitcode, ok."
    fi
    end_verbatim
}

function testSetLdapConfigWrongFiles()
{
    local exitcode

    print_title "Setting LDAP Configuration with Wrong Files"
    cat <<EOF | paragraph
  This test will try to set the LDAP configuration with the wrong LDAP protocol
  version.
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json  \\
        --enabled              \\
        --ca-cert-file "notok" \\
        |                      \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    emit_ldap_settings_json \
        --enabled \
        --ca-cert-file "notok" \
        |  \
        s9s controller         \
            --set-ldap-config  \
            --cmon-user=system \
            --password=secret  \
            --print-request    \
            --print-json       \
            --color=always

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
    cat <<EOF
    # echo "not a json string" \\
        |  \\
        s9s controller         \\
            --cmon-user=system \\
            --password=secret  \\
            --set-ldap-config  \\
            --print-request    \\
            --print-json       \\
            --color=always
EOF

    echo "not a json string" \
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
            --print-request    \
            --print-json       \
            --color=always

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
        ".ldap_configuration.groupMappings[0].cmonGroupName"  "myldapgroup" \
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

    #
    # Testing the user once.
    #
    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    check_exit_code_no_job $?
   
    #
    # Again...
    # 
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

    #
    # Now with uppercase letters.
    #
    username="USERNAME"
    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    check_exit_code_no_job $?

    lines=$(s9s user --list --long --batch);
    if echo "$lines" | grep -q "USERNAME"; then
        failure "The 'USERNAME' user should not be created."
    else
        success "  o No 'USERNAME' user is created, ok."
    fi
    
    if echo "$lines" | grep -q "username"; then
        success "  o The 'username' user is created, ok."
    else
        failure "The 'username' user should be created."
    fi

    end_verbatim
}

function testLdapUser1Again()
{
    local username="username"

    print_title "Checking LDAP Authentication Again"

    cat <<EOF | paragraph
  This check is executed after we tried to set the LDAP configuration. The
  original configuration should still be working and is checked here.
EOF

    begin_verbatim

    mys9s user \
        --list \
        --long \
        --cmon-user="$username" \
        --password=p

    check_exit_code_no_job $?
    end_verbatim
}

function testLdapUser1Fail()
{
    local username="username"

    print_title "Checking That LDAP User '$username' Can Not Log In"

    cat <<EOF | paragraph
  This test checks the LDAP authentication using the simple name. The user
  should not be able to authenticate.
EOF

    begin_verbatim

    #
    # 
    #
    mys9s user --list --long

    for n in 1 2 3 4 5 6; do
        mys9s user \
            --list \
            --long \
            --cmon-user="$username" \
            --password=p

        if [ $? == 0 ]; then
            failure "The user should not be able to log in."
        else
            success "  o The user could not authenticate, ok."
        fi
    done

    mys9s user --stat $username

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

    # This is for reference when debugging.
    #runFunctionalTest testCreateLdapConfig

    # This is what we test.
    runFunctionalTest testSetLdapConfigEnabled

    runFunctionalTest testGetLdapConfig2
    runFunctionalTest testLdapUser1

    runFunctionalTest testSetLdapConfigDisabled
    runFunctionalTest testGetLdapConfig3
    runFunctionalTest testLdapUser1Fail

    runFunctionalTest testSetLdapConfigWrongUserSearch
    runFunctionalTest testLdapUser1Fail


    # First setting the LDAP to a working configuration, then try various wrong
    # configurations, then we check the the original good configuration is kept
    # and keeps working.
    runFunctionalTest testSetLdapConfigEnabled
    runFunctionalTest testSetLdapConfigSyntaxError
    runFunctionalTest testSetLdapConfigNoAccess
    runFunctionalTest testSetLdapConfigEmptyUrl
    runFunctionalTest testSetLdapConfigWrongUrl
    runFunctionalTest testSetLdapConfigWrongPassword
    runFunctionalTest testSetLdapConfigWrongAdmin
    runFunctionalTest testSetLdapConfigWrongFiles
    runFunctionalTest testSetLdapConfigWrongProtocol

    runFunctionalTest testLdapUser1Again
fi

runFunctionalTest --force endTests

