#!/bin/bash

VERSION=`dpkg-query -W -f='${Version}\n' xserver-xorg-core | cut -d: -f 2 | cut -d'-' -f 1`

mkdir -p external/Source
cd external/Source

sudo apt-get build-dep xorg-server xserver-xorg-core
apt-get source xserver-xorg-core
ln -s xorg-server-${VERSION} xorg-server
