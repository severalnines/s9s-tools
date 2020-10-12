#
# This is an include file that contains LDAP related tests that are executed
# from multiple scripts.
#

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

