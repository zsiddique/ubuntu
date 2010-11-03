#!/bin/bash -e

# Set to "-sa" when the orig.tar.bz2 needs to be uploaded
DEBUILD_PARAMETER="-sa"
PPA_REPOSITORY="ppa:skettler/php"

PACKAGE=`basename "${PWD}"`
DIRECTORY=`ls -1d "${PACKAGE}"-* | tail -1`
PROGRAM_VERSION=`echo "${DIRECTORY}" | sed "s/${PACKAGE}-//"`
PACKAGE_VERSION=`cd "${DIRECTORY}"; dpkg-parsechangelog|grep "^Version: "|sed "s/^Version: //"`

echo ${PACKAGE}
echo ${DIRECTORY}
echo ${PROGRAM_VERSION}
echo ${PACKAGE_VERSION}

if [ ! -e ${PACKAGE}_${PACKAGE_VERSION}_source.changes ]; then
    ( cd ${DIRECTORY} && debuild -S ${DEBUILD_PARAMETER} ); [ $? -eq 0 ]
    dput ${PPA_REPOSITORY} ${PACKAGE}_${PACKAGE_VERSION}_source.changes
fi

