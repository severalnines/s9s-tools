#
# This is an include file that contains LDAP related tests that are executed
# from multiple scripts.
#
export LDAP_URL="ldap://ldap.homelab.local:389"

#
# This function will print a working LDAP configuration file to the standard
# output.
#
function emit_ldap_config()
{
    local ldap_url=""

    while [ -n "$1" ]; do
        case "$1" in 
            --ldap-url)
                ldap_url="$2"
                shift 2
                ;;

            *)
                failure "emit_ldap_config(): Unknown option '$1'."
                ;;
        esac
    done

    cat <<EOF
#
# Basic settings of the LDAP configuration.
#
enabled                = true
ldapServerUri          = "$ldap_url"
ldapAdminUser          = "cn=admin,dc=homelab,dc=local"
ldapAdminPassword      = "p"

ldapUserSearchRoot     = "dc=homelab,dc=local"
ldapGroupSearchRoot    = "dc=homelab,dc=local"

#
# Some more sophisticated values that will control how to interpret 
# the data from the LDAP server.
#
[ldap_settings]
ldapUsernameAttributes = "cn"
ldapRealnameAttributes = "displayName,cn"
ldapEmailAttributes    = "mail"
ldapMemberAttributes   = "memberUid"

#
# Group mappings that will map the groups on the LDAP server to the
# groups on the Cmon Controller.
#
[mapping1]
ldapGroupId            = "ldapgroup"
cmonGroupName          = "ldapgroup"

[mapping2]
ldapGroupId            = "cmonusers"
cmonGroupName          = "users"
EOF

    return 0
}

#
# Creating the group or groups needed by the LDAP test scripts.
#
function testCreateLdapGroup()
{
    local groupName="ldapgroup"

    print_title "Creating LDAP Group"

    cat << EOF | paragraph
    LDAP tests need to have some group(s) for the LDAP users created in advance.
    This test will create the necessary group(s) for the test scripts.
EOF
    
    begin_verbatim
 
    while [ -n "$1" ]; do
        case "$1" in
            --group-name)
                groupName="$2"
                shift 2
                ;;

            *)
                failure "testCreateLdapGroup(): Unknown option '$1'."
                return 1
        esac
    done

    if [ -n "$groupName" ]; then
        success "  o Will create group '$groupName'"
    else
        failure "No group name is provided."
    fi

    mys9s group \
        --cmon-user="system" \
        --password="secret" \
        --create $groupName 
    check_exit_code_no_job $?

    check_group \
        --group-name   "$groupName"    

    mys9s group --list --long
    end_verbatim  
}

#
# Checking the /.runtime/controller CDT file to see if there is an LDAP support.
# In the current version of the controller LDAP support is mandatory.
# 
function testLdapSupport()
{
    print_title "Checking LDAP Support"
    cat <<EOF
  This test checks if the controller has LDAP support.

EOF

    begin_verbatim
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

    end_verbatim
}

# 
# Distinguished name logins, these are not supported any more and so these tests
# are probably deprecated.
#
function testLdapUserDn()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
    cat <<EOF | paragraph
  This test will check teh LDAP authentication using the distinguished name at
  the login. 
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

# 
# Distinguished name logins, these are not supported any more and so these tests
# are probably deprecated.
#
function testLdapUserDnSecond()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
    cat <<EOF | paragraph
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

# 
# Distinguished name logins, these are not supported any more and so these tests
# are probably deprecated.
#
function testLdapUserDnOne()
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

# 
# Distinguished name logins, these are not supported any more and so these tests
# are probably deprecated.
#
function testLdapObjectDn()
{
    print_title "Checking LDAP Authentication with Distinguished Name"
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


