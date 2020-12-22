#
# This is an include file that contains LDAP related tests that are executed
# from multiple scripts.
#

#
# Creating the group or groups needed by the LDAP test scripts.
#
function testCreateLdapGroup()
{
    print_title "Creating LDAP Group"
    cat << EOF | paragraph
    LDAP tests need to have some group(s) for the LDAP users created in advance.
    This test will create the necessary group(s) for the test scripts.
EOF
 
    begin_verbatim
    mys9s group --create ldapgroup
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


