#!/bin/bash
# David Kedves <kedazo@gmail.com>

if [ ! -d debian ]; then
    echo "Please execute this script inside the project directory."
    exit 1
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
debchange --newversion "${VERSION}-release1" "Release ${VERSION}."
# rpm project version update
sed "s/%changelog/%changelog\n* ${RPMDATE} ${DEBFULLNAME} <${DEBEMAIL}> ${VERSION}\n- Release ${VERSION}./" -i project.spec

#dpkg-buildpackage -rfakeroot -S --no-sign

