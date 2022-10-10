#
# A file to cmon user related tests that are shared between functional tests.
#

function createUserSisko()
{
    local config_dir="$HOME/.s9s"
    local myself

    #
    #
    #
    print_title "Creating a User"
    begin_verbatim

    mys9s user \
        --create \
        --title="Captain" \
        --first-name="Benjamin" \
        --last-name="Sisko"   \
        --email-address="sisko@ds9.com" \
        --generate-key \
        --group=ds9 \
        --create-group \
        --batch \
        "sisko"
    retcode=$?
    
    cat /tmp/LAST_COMMAND_OUTPUT

    check_exit_code_no_job $retcode

    ls -lha "$config_dir"

    if [ ! -f "$config_dir/sisko.key" ]; then
        failure "Secret key file 'sisko.key' was not found."
    else
        success "  o Secret key file 'sisko.key' was found."
    fi

    if [ ! -f "$config_dir/sisko.pub" ]; then
        failure "Public key file 'sisko.pub' was not found."
    else
        success "  o Public key file 'sisko.pub' was found."
    fi

    myself=$(s9s user --whoami)
    if [ "$myself" != "$S9STEST_USER" ]; then
        failure "Whoami returns $myself instead of $S9STEST_USER."
    else
        success "  o Whoami returns $myself, OK."
    fi

    end_verbatim
}

