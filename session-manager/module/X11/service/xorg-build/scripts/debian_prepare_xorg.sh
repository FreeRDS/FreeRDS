#!/bin/bash

#
# Determine the actual package name and version for xserver-xorg-core
#
PACKAGE=`dpkg-query -W | awk '/xserver-xorg-core/ { print $1 }'`
VERSION=`dpkg-query -W | awk '/xserver-xorg-core/ { print $2 }' | cut -d: -f 2 | cut -d'-' -f 1`
SUFFIX=`echo ${PACKAGE} | sed 's/xserver-xorg-core//'`

echo "package=${PACKAGE}"
echo "version=${VERSION}"
echo "suffix=${SUFFIX}"

mkdir -p external/Source
cd external/Source

#sudo apt-get build-dep xorg-server${SUFFIX} ${PACKAGE}
apt-get source xorg-server${SUFFIX}
ln -s xorg-server${SUFFIX}-${VERSION} xorg-server

cd ../..
