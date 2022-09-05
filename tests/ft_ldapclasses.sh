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

OPTION_LDAP_CONFIG_FILE="/etc/cmon-ldap.cnf"

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
    local ldapUserClassName="objectclass=*"
    local ldapGroupClassName=""

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

            --ldap-user-class-name)
                ldapUserClassName="$2"
                shift 2
                ;;

            --ldap-group-class-name)
                ldapGroupClassName="$2"
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
      "ldapGroupClassName": "$ldapGroupClassName",
      "ldapGroupIdAttributes": null,
      "ldapGroupNameAttribute": null,
      "ldapMemberAttributes": "memberUid",
      "ldapNetworkTimeout": null,
      "ldapProtocolVersion": $ldapProtocolVersion,
      "ldapQueryTimeLimit": null,
      "ldapRealnameAttributes": "displayName,cn",
      "ldapUserClassName": "$ldapUserClassName",
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

function testSetLdapConfig()
{
    print_title "Setting LDAP Configuration"
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
        |  \
        s9s controller         \
            --cmon-user=system \
            --password=secret  \
            --set-ldap-config  \
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
            --color=always 

    check_exit_code_no_job $?
    end_verbatim
}

function testSetLdapConfigWrongUserClassName()
{
    print_title "Setting LDAP Configuration with Wrong User Search Root"
    cat <<EOF | paragraph
    ...
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json \\
        --ldap-user-class-name "objectclass=wrong" \\
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
        --ldap-user-class-name "objectclass=wrong" \
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

function testSetLdapConfigWrongGroupClassName()
{
    print_title "Setting LDAP Configuration with Wrong User Search Root"
    cat <<EOF | paragraph
    ...
EOF

    begin_verbatim
    cat <<EOF
    # emit_ldap_settings_json \\
        --ldap-group-class-name "objectclass=wrong" \\
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
        --ldap-group-class-name "objectclass=wrong" \
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

function testLdapUser1Succeeds()
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
    #mys9s user --list --long

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

    #mys9s user --stat $username

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
    runFunctionalTest testCreateLdapGroup --group-name myldapgroup

    runFunctionalTest testSetLdapConfig
    runFunctionalTest testLdapUser1Succeeds
    
    runFunctionalTest testSetLdapConfigWrongUserClassName
    runFunctionalTest testLdapUser1Fail
    
    runFunctionalTest testSetLdapConfigWrongGroupClassName
    runFunctionalTest testLdapUser1Fail
    
    runFunctionalTest testSetLdapConfig
    runFunctionalTest testLdapUser1Succeeds

    #runFunctionalTest testSetLdapConfigWrongUserSearch
    #runFunctionalTest testLdapUser1Fail
fi

runFunctionalTest --force endTests

