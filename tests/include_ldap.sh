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

function testLdapGroup()
{
    local username="cn=lpere,cn=ldapgroup,dc=homelab,dc=local"

    print_title "Checking LDAP Groups"
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

#
# Checking the successful authentication of an LDAP user with a simple name.
#
function testLdapUserSimple()
{
    print_title "Checking LDAP Authentication with Username"
    cat <<EOF | paragraph
  This test checks the LDAP authentication using the simple name. The user
  should be able to authenticate.
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

    check_user \
        --user-name    "username"  \
        --full-name    "firstname lastname" \
        --email        "username@domain.hu" \
        --cdt-path     "/" \
        --group        "ldapgroup" \
        --dn           "cn=username,dc=homelab,dc=local" \
        --origin       "LDAP"

    end_verbatim
}

#
# Testing a successful authentication of an LDAP user with a distinguished 
# name.
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

