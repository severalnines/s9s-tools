#!/bin/bash
# a script to make the s9s-tools easily releasable
# Author: David Kedves <kedazo@severalnines.com>
if [ "${PUBLISHING_REPO}" == "TESTING" ]; then
    REPONAME='home:severalnines/s9s-tools-testing'
else
    REPONAME='home:severalnines/s9s-tools'
fi
echo "Comitting source files to OBS repository: ${REPONAME}"

CURRDIR=`pwd`
SRCDIR=${CURRDIR}/../build

if ! test -f .git/config; then
  echo "You must start this script inside the s9s-tools source directory."
  exit 1
fi

if command -v osc; then
  echo "OSC `osc --version` is installed."
else
  echo "OSC is not installed (apt-get install osc)."
  exit 1
fi

echo "Cleaning up."
git reset --hard
git clean -dfx
git pull
rm -rf ${SRCDIR}
mkdir -p ${SRCDIR}
cp -fva ${CURRDIR} ${SRCDIR}/s9s-tools-master

# remove old debian sources and binary files
echo "Building source packages for deb building."
cd ${SRCDIR}/s9s-tools-master
dpkg-buildpackage -rfakeroot -S
echo "Compressing sources for rpm building."
cd ${SRCDIR}
zip -r master.zip s9s-tools-master -xs9s-tools-master/.git/\* -x\*screenshots\*
cp -fva ${CURRDIR}/project.spec ${SRCDIR}

echo "Cleaning up for upload."
rm -rf ${SRCDIR}/s9s-tools-master

cd ${SRCDIR}
osc checkout "${REPONAME}"
rm -rfv ${REPONAME}/*.*
cp -fva ${SRCDIR}/*.* ${REPONAME}/
cd ${REPONAME}
osc addremove
osc commit --message="Release `date +%Y-%m-%d_%H%I`"

