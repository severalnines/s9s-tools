#!/bin/bash
#
# This is the s9s-tools install script using the testing repository. It installs the s9s CLI for not released versions.
# Copyright 2017-2023 severalnines.com
#

dist="Unknown"
distversion=""
regex_lsb="Description:[[:space:]]*([^ ]*)"
regex_etc="/etc/(.*)[-_]"

log_msg() {
    LAST_MSG="$1"
    echo "${LAST_MSG}"
}

log_msg "s9s-tools installer using testing repository."

if [[ $EUID -ne 0 ]]; then
   log_msg "This script must be run as root"
   exit 1
fi

do_lsb() {
    os_codename=$(lsb_release -sc)
    lsb=`lsb_release -d`
    [[ $lsb =~ $regex_lsb ]] && dist=${BASH_REMATCH[1]} ; return 0
    return 1
}

detect_suse_version() {
    dist="suse"
    # we only build for 15.3, 15.4 and tumbleweed for now
    distversion="15.3" # as a fallback
    if grep '15\.4' /etc/os-release >/dev/null 2>/dev/null; then
        distversion="15.4"
    elif grep tumbleweed /etc/os-release >/dev/null 2>/dev/null; then
        distversion="tumbleweed"
    fi
}

do_release_file() {
    etc_files=`ls /etc/*[-_]{release,version} 2>/dev/null`
    for file in $etc_files; do
        # /etc/SuSE-release is deprecated and will be removed in the future, use /etc/os-release instead
        if [[ $file == "/etc/os-release" ]]; then
            if grep SLE $file >/dev/null 2>/dev/null; then
                detect_suse_version
                break
            elif grep opensuse $file >/dev/null 2>/dev/null; then
                detect_suse_version
                break
            else
                egrep -q "(VERSION=.*7\.|platform:el7)" $file \
                    >/dev/null 2>/dev/null && dist="redhat" && distversion=7 && break
                egrep -q "(VERSION=.*8\.|platform:el8)" $file \
                    >/dev/null 2>/dev/null && dist="redhat" && distversion=8 && break
                egrep -q "(VERSION=.*9\.|platform:el9)" $file \
                    >/dev/null 2>/dev/null && dist="redhat" && distversion=9 && break
            fi
        fi
        if [[ $file =~ $regex_etc ]]; then
            dist=${BASH_REMATCH[1]}
            # tolower. bash subs only in bash 4.x
            #dist=${dist,,}
            dist=$(echo $dist | tr '[:upper:]' '[:lower:]')
            if [[ $dist == "redhat" ]] || [[ $dist == "red" ]] || [[ $dist == "fedora" ]]; then
                grep -q " 7." $file >/dev/null 2>/dev/null && distversion=7 && break
                grep -q " 8." $file >/dev/null 2>/dev/null && distversion=8 && break
                grep -q " 9." $file >/dev/null 2>/dev/null && distversion=9 && break
                grep -q "21" $file >/dev/null 2>/dev/null && distversion=7 && break
            fi
        fi
    done
    [[ -f /etc/centos-release ]] && CENTOS=1
}

log_msg ">= Detectiong OS ..."
if [[ `command -v lsb_release` ]]; then
    do_lsb
    [[ $? -ne 0 ]] && do_release_file
else
    do_release_file
fi

dist=$(echo $dist | tr '[:upper:]' '[:lower:]')
case $dist in
    debian) dist="debian";;
    ubuntu) dist="debian";;
    red)    dist="redhat";;
    redhat) dist="redhat";;
    centos) dist="redhat";;
    fedora) dist="redhat";;
    rocky)  dist="redhat";;
    alma)   dist="redhat";;
    oracle) dist="redhat";;
    system) dist="redhat";; # amazon ami
    suse)   dist="suse";;
    *) log_msg "=> This Script is currently not supported on $dist. You can try changing the distribution check."; exit 1
esac

add_s9s_commandline_apt() {
    log_msg "=> Adding APT repository ..."
    apt-get -yq install wget || true
    wget -qO - https://build.opensuse.org/projects/home:severalnines/public_key | apt-key add -

    # Available distros: wheezy, jessie, precise, trusty, xenial, yakkety, zesty
    wget -qO - http://repo.severalnines.com/s9s-tools-testing/${os_codename}/Release.key | apt-key add -
    echo "deb http://repo.severalnines.com/s9s-tools-testing/${os_codename}/ ./" | tee /etc/apt/sources.list.d/s9s-tools.list
    # update repositories
    apt -yq update -oAcquire::AllowReleaseInfoChange::Origin=true -oAcquire::AllowReleaseInfoChange::Label=true
}

add_s9s_commandline_yum() {
    log_msg "=> Adding YUM repository ..."
    rpm --import https://build.opensuse.org/projects/home:severalnines/public_key || true

    repo_source_file=/etc/yum.repos.d/s9s-tools.repo
    if [[ -z $CENTOS ]]; then
        REPO="RHEL_7"
        [[ $distversion == "8" ]] && REPO="RHEL_8"
        [[ $distversion == "9" ]] && REPO="RHEL_9"
    else
        REPO="CentOS_7"
        [[ $distversion == "8" ]] && REPO="CentOS_8"
        [[ $distversion == "9" ]] && REPO="CentOS_9"
    fi
    # after dist upgrade or errors we must re-recreate this repo file
    grep -q ${REPO} $repo_source_file >/dev/null 2>/dev/null || rm -f $repo_source_file
    if [[ ! -e $repo_source_file ]]; then
        cat > $repo_source_file << EOF
[s9s-tools]
name=s9s-tools (${REPO})
type=rpm-md
baseurl=http://repo.severalnines.com/s9s-tools-testing/${REPO}
gpgcheck=1
gpgkey=http://repo.severalnines.com/s9s-tools-testing/${REPO}/repodata/repomd.xml.key
enabled=1
EOF
        log_msg "=> Added ${repo_source_file}"
    else
        log_msg "=> Repo file $repo_source_file already exists"
    fi
}

add_s9s_commandline_zypper() {
    log_msg "=> Adding Zypper repository ..."
    rpm --import https://build.opensuse.org/projects/home:severalnines/public_key || true

    repo_source_file=/tmp/s9s-tools.repo
    cat > $repo_source_file << EOF
[s9s-tools]
name=s9s-tools (Suse ${distversion})
type=rpm-md
baseurl=http://repo.severalnines.com/s9s-tools-testing/${distversion}
gpgcheck=1
gpgkey=http://repo.severalnines.com/s9s-tools-testing/${distversion}/repodata/repomd.xml.key
enabled=1
EOF
    # make sure curl or wget is available
    command -v curl || zypper -n install --no-confirm curl || command -v wget || zypper -n install --no-confirm wget
    if command -v curl; then
        curl "http://repo.severalnines.com/s9s-tools-testing/${distversion}/repodata/repomd.xml.key" -o/tmp/s9s-tools.asc
        rpm --import /tmp/s9s-tools.asc
    elif command -v wget; then
        wget "http://repo.severalnines.com/s9s-tools-testing/${distversion}/repodata/repomd.xml.key" -O/tmp/s9s-tools.asc
        rpm --import /tmp/s9s-tools.asc
    else
        rpm --import "http://repo.severalnines.com/s9s-tools-testing/${distversion}/repodata/repomd.xml.key"
    fi
    zypper -n addrepo --refresh ${repo_source_file}
    log_msg "=> Added ${repo_source_file}"
}


install_s9s_commandline() {
    log_msg "=> Installing s9s-tools ..."
    if [[ $dist == "redhat" ]]; then
        yum -y install s9s-tools || dnf -y install s9s-tools
    elif [[ $dist == "debian" ]]; then
        apt-get -y install s9s-tools
    elif [[ $dist == "suse" ]]; then
        zypper -n install --no-confirm s9s-tools
    fi
    [[ $? -ne 0 ]] && log_msg "Unable to install s9s-tools"
}

revive_user() {
    # a method to create a new temporary admin user to revive
    # an existing suspended user (due to previus bugs we had)

    echo "=> Checking user $1."
    s9s user --whoami
    if (( $? == 3 )); then
        unset S9S_IGNORE_CONFIG
        export S9S_USER_CONFIG="/dev/null"
        echo -n "=> Re-enabling suspended user '$1'. "
        s9s user --enable "$1"
        unset S9S_USER_CONFIG
    fi

    unset S9S_SYSTEM_CONFIG
    unset S9S_USER_CONFIG
}

create_local_s9s_user() {
    # clean up the environment
    unset S9S_IGNORE_CONFIG
    unset S9S_SYSTEM_CONFIG
    unset S9S_USER_CONFIG

    # this is for s9s_error_reporter, make sure we have a user created
    # NOTE: this will run only if we have root credentials
    log_msg "=> Ensuring s9s has an initial user."

    # this is a new thing, if the system s9s conf is readable and has
    # a working user (generated by cmon) we should use that simply
    if [ -z "$SUDO_USER" -a -r /etc/s9s.conf ]; then
        # check if it works, then just return
        s9s user --whoami && return
    fi

    unset cmon_user
    if [ -r ~/.s9s/s9s.conf ]; then
        source ~/.s9s/s9s.conf &>/dev/null
    fi

    # verify if the specified user works
    if [ -n "$cmon_user" ]; then
        # in case of suspended user, lets try to re-enable it
        revive_user "$cmon_user"

        s9s user --whoami >/dev/null 2>/dev/null
        if (( $? == 3 )); then
            # access denied, backup the ~/.s9s directory and create a new user
            mv ~/.s9s ~/.s9s_bak_`date +%F_%H%M`

            # if the system config works here, lets just return we are good
            if [ -r /etc/s9s.conf ]; then
                s9s user --whoami && return
            fi

            # okay, creating a new user then
            cmon_user=""
        fi
    fi

    # check if s9s.conf doesn't have a user configured, then create one
    if [ -z "$cmon_user" ]; then
        if [ -z "$SUDO_USER" ]; then
            cmon_user="root"
        else
            cmon_user="$SUDO_USER"
        fi

        echo "=> Creating user ${cmon_user}."
        # new s9s CLI requires the username defined alone without --cmon-user
        # only the new one has build info in --version:
        if s9s --version | grep -i build >/dev/null; then
            s9s user --create --generate-key --group=admins --controller="https://localhost:9501" "$cmon_user"
        else
            s9s user --create --generate-key --group=admins --controller="https://localhost:9501" --cmon-user="$cmon_user"
        fi

        # in case of suspended user, lets try to re-enable it
        revive_user "$cmon_user"

        chown -R "$cmon_user" ~/.s9s
    fi

    log_msg "=> s9s-tools user has been created."
}

if [[ "$dist" == "debian" ]]; then
    command -v wget &>/dev/null
    [[ $? -ne 0 ]] && log_msg "=> Installing wget ..." && apt-get install -y wget
    command -v gpg &>/dev/null
    [[ $? -ne 0 ]] && log_msg "=> Installing gnupg ..." && ( apt-get install -y gnupg || apt-get install -y gnupg2 || apt-get install -y gpg )
    command -v lsb_release &>/dev/null
    [[ $? -ne 0 ]] && log_msg "=> Installing lsb_release ..." && apt-get install -y lsb_release
    add_s9s_commandline_apt
fi

if [[ "$dist" == "redhat" ]]; then
    add_s9s_commandline_yum
fi

if [[ "$dist" == "suse" ]]; then
    add_s9s_commandline_zypper
fi

install_s9s_commandline
create_local_s9s_user

log_msg "=> s9s-tools installation has finished."
exit 0

