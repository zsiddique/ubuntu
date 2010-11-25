#!/bin/bash -e

# Set to "-sa" when the orig.tar.bz2 needs to be uploaded
DEBUILD_PARAMETER="-sa"
PPA_REPOSITORY="ppa:skettler"
GIT_UBUNTU_RELEASE="lucid"

PPA_DIRECTORY=`cd .. && basename "${PWD}"`
PACKAGE=`basename "${PWD}"`
DIRECTORY=`ls -1d "${PACKAGE}"-* | tail -1`
PROGRAM_VERSION=`echo "${DIRECTORY}" | sed "s/${PACKAGE}-//"`

. /etc/lsb-release

( cd ${DIRECTORY} && fakeroot ./debian/rules clean )

if [ "${DISTRIB_CODENAME}" != "${GIT_UBUNTU_RELEASE}" ]; then
    sed -i "s~${GIT_UBUNTU_RELEASE}~${DISTRIB_CODENAME}~g" ${DIRECTORY}/debian/changelog
fi

PACKAGE_VERSION=`cd "${DIRECTORY}"; dpkg-parsechangelog|grep "^Version: "|sed "s/^Version: //"`
( cd ${DIRECTORY} && debuild -S ${DEBUILD_PARAMETER} ); [ $? -eq 0 ]
dput ${PPA_REPOSITORY}/${PPA_DIRECTORY} ${PACKAGE}_${PACKAGE_VERSION}_source.changes
rm -f ${PACKAGE}_${PACKAGE_VERSION}.dsc ${PACKAGE}_${PACKAGE_VERSION}_source.*
rm -f ${PACKAGE}_${PACKAGE_VERSION}.tar.gz ${PACKAGE}_${PACKAGE_VERSION}.tar.bz2

if [ "${DISTRIB_CODENAME}" != "${GIT_UBUNTU_RELEASE}" ]; then
    sed -i "s~${DISTRIB_CODENAME}~${GIT_UBUNTU_RELEASE}~g" ${DIRECTORY}/debian/changelog
fi

