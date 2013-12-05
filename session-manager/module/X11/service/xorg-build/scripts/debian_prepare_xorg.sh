#!/bin/bash

#
# Grab the actual package name and version for xserver-xorg-core.  Note
# that this name can differ on various Linux distros (e.g., the package
# is xserver-xorg-core-lts-raring on Ubuntu 12.04 LTS).
#
PACKAGE=`dpkg-query -W | awk '/xserver-xorg-core/ { print $1 }'`
VERSION=`dpkg-query -W | awk '/xserver-xorg-core/ { print $2 }' | cut -d: -f 2 | cut -d'-' -f 1`
SUFFIX=`echo ${PACKAGE} | sed 's/xserver-xorg-core//'`

echo "package=${PACKAGE}"
echo "version=${VERSION}"
echo "suffix=${SUFFIX}"

mkdir -p external/Source
cd external/Source

#
# This controls whether we use apt-get or grab sources directly
# from xorg.freedesktop.org.
#
USE_APT_GET=1

if [ ${USE_APT_GET} == 1 ]; then

#
# Use apt-get to grab sources
#
#sudo apt-get build-dep xorg-server${SUFFIX} ${PACKAGE}
sudo apt-get build-dep xorg-server${SUFFIX}
apt-get source xorg-server${SUFFIX}
ln -s xorg-server${SUFFIX}-${VERSION} xorg-server

else

#
# Grab sources directly from xorg.freedesktop.org
#
XORG_NAME=xorg-server-${VERSION}

wget http://xorg.freedesktop.org/releases/individual/xserver/${XORG_NAME}.tar.gz
tar zxvf ${XORG_NAME}.tar.gz
ln -s ${XORG_NAME} xorg-server
rm ${XORG_NAME}.tar.gz

fi
