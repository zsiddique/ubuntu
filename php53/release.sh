#!/bin/bash -e

# Set to "-sa" when the orig.tar.bz2 needs to be uploaded
DEBUILD_PARAMETER="-sa"
PPA_REPOSITORY="ppa:skettler/php"
GIT_UBUNTU_RELEASE="lucid"

PACKAGE=`basename "${PWD}"`
DIRECTORY=`ls -1d "${PACKAGE}"-* | tail -1`
PROGRAM_VERSION=`echo "${DIRECTORY}" | sed "s/${PACKAGE}-//"`

. /etc/lsb-release

( cd ${DIRECTORY} && ./debian/rules clean ); [ $? -eq 0 ]

if [ "${DISTRIB_CODENAME}" != "${GIT_UBUNTU_RELEASE}" ]; then
    sed -i "s~${GIT_UBUNTU_RELEASE}~${DISTRIB_CODENAME}~g" ${DIRECTORY}/debian/changelog
fi

PACKAGE_VERSION=`cd "${DIRECTORY}"; dpkg-parsechangelog|grep "^Version: "|sed "s/^Version: //"`
if [ ! -e ${PACKAGE}_${PACKAGE_VERSION}_source.changes ]; then
    ( cd ${DIRECTORY} && debuild -S ${DEBUILD_PARAMETER} ); [ $? -eq 0 ]
    dput ${PPA_REPOSITORY} ${PACKAGE}_${PACKAGE_VERSION}_source.changes
fi

if [ "${DISTRIB_CODENAME}" != "${GIT_UBUNTU_RELEASE}" ]; then
    sed -i "s~${DISTRIB_CODENAME}~${GIT_UBUNTU_RELEASE}~g" ${DIRECTORY}/debian/changelog
fi

