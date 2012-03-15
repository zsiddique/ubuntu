#!/bin/bash

# Set to "-sa" when the orig.tar.bz2 needs to be uploaded
DEBUILD_PARAMETER=""
PPA_REPOSITORY="ppa:skettler"
GIT_UBUNTU_RELEASE="lucid"

PPA_DIRECTORY=`cd .. && basename "${PWD}"`
PACKAGE=`basename "${PWD}"`
DIRECTORY=`ls -1d "${PACKAGE}"-* 2> /dev/null | tail -1`
PROGRAM_VERSION=`echo "${DIRECTORY}" | sed "s/${PACKAGE}-//"`

if [ -z "$PROGRAM_VERSION" ]; then
    echo "Unable to detect version, aborting"
    exit 1
fi

if [ $# = 0 ]; then
    echo "Usage:"
    echo "    $0 <distribution>"
    exit 1
fi

case $1 in
    hardy|lucid)
        DISTRIB_CODENAME=$1
        ;;
    *)
        echo "Unknown distribution $1, aborting"
        exit 1
        ;;
esac

( cd ${DIRECTORY} && ./debian/rules clean ); [ $? -eq 0 ]

if [ "${DISTRIB_CODENAME}" != "${GIT_UBUNTU_RELEASE}" ]; then
    sed -i "s~${GIT_UBUNTU_RELEASE}~${DISTRIB_CODENAME}~g" ${DIRECTORY}/debian/changelog ${DIRECTORY}/debian/control
fi

PACKAGE_VERSION=`cd "${DIRECTORY}"; dpkg-parsechangelog|grep "^Version: "|sed "s/^Version: //"|sed "s/^[0-9]*://"`
( cd ${DIRECTORY} && debuild -S ${DEBUILD_PARAMETER} ); [ $? -eq 0 ] && dput ${PPA_REPOSITORY}/${PPA_DIRECTORY} ${PACKAGE}_${PACKAGE_VERSION}_source.changes
rm -f ${PACKAGE}_${PACKAGE_VERSION}.dsc ${PACKAGE}_${PACKAGE_VERSION}_source.*
rm -f ${PACKAGE}_${PACKAGE_VERSION}.tar.gz ${PACKAGE}_${PACKAGE_VERSION}.tar.bz2

if [ "${DISTRIB_CODENAME}" != "${GIT_UBUNTU_RELEASE}" ]; then
    sed -i "s~${DISTRIB_CODENAME}~${GIT_UBUNTU_RELEASE}~g" ${DIRECTORY}/debian/changelog ${DIRECTORY}/debian/control
fi

