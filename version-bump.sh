#!/bin/bash
# David Kedves <kedazo@gmail.com>

if [ ! -d debian ]; then
    echo "Please execute this script inside the project directory."
    exit 1
fi

if [ "${PUBLISHING_REPO}" == "TESTING" ]; then
    VERSION_SUFFIX='testing'
    RELEASE_LOG_LINE='Testing release'
else
    VERSION_SUFFIX='release1'
    RELEASE_LOG_LINE='Release'
fi

# remove old source packages
# rm -fv ../s9s-tools*

export LC_ALL=C
export LC_DATE=C
DEBEMAIL="support@severalnines.com"
DEBFULLNAME="Severalnines"
export EMAIL=${DEBEMAIL}
export DEBFULLNAME=${DEBFULLNAME}
VERSION="1.9.`date +%Y%m%d%H`"
# RPM needs the date/time in a very specific format
RPMDATE=`date +"%a %b %_d %Y"`

echo "VERSION:"
echo "${VERSION}" | tee version.txt

# debian changelog update
debchange --newversion "${VERSION}-${VERSION_SUFFIX}" "${RELEASE_LOG_LINE} ${VERSION}."
# rpm project version update
sed "s/%changelog/%changelog\n* ${RPMDATE} ${DEBFULLNAME} <${DEBEMAIL}> ${VERSION}\n- ${RELEASE_LOG_LINE} ${VERSION}./" -i project.spec

#dpkg-buildpackage -rfakeroot -S --no-sign

