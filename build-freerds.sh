#! /bin/bash
#
#===========================================================================
#
# build-freerds.sh
#
# This script builds and installs FreeRDS on a clean Ubuntu 12.04 LTS
# installation.  Follow these steps:
#
#   1. Install Ubuntu 12.04 LTS.
#   2. Apply Ubuntu software updates.
#   3. Open a Terminal window.
#   4. Download this script.
#   5. Run the command "chmod +x build-freerds.sh".
#   6. Run the command "./build-freerds.sh
#
#===========================================================================

GIT_ROOT_DIR=~/git/vworkspace
FREERDP_GIT=https://github.com/awakecoding/FreeRDP.git
FREERDS_GIT=https://github.com/bmiklautz/FreeRDS.git
FREERDS_INSTALL_DIR=/opt/FreeRDS

#
# Fetch sources from GitHub
#
sudo apt-get install -y git-core

if [ ! -d $GIT_ROOT_DIR ]; then
  mkdir -p $GIT_ROOT_DIR
fi

if [ ! -d $GIT_ROOT_DIR/FreeRDP ]; then
  pushd $GIT_ROOT_DIR
  git clone $FREERDP_GIT
  popd
fi

if [ ! -d $GIT_ROOT_DIR/FreeRDP/server/FreeRDS ]; then
  pushd $GIT_ROOT_DIR/FreeRDP/server
  git clone $FREERDS_GIT
  popd
fi

#
# Install the correct version of CMake
#
sudo apt-get install -y build-essential

CMAKE_VERSION=2.8.12
CMAKE_FOLDER=cmake-$CMAKE_VERSION
CMAKE_ARCHIVE=$CMAKE_FOLDER.tar.gz
CMAKE_URL=http://www.cmake.org/files/v2.8/$CMAKE_ARCHIVE

RESULT=`cmake --version`
if [ "$RESULT" != "cmake version $CMAKE_VERSION" ]; then
  # Download compressed archive
  pushd ~/Downloads
  wget $CMAKE_URL
  popd

  # Unpack compressed archive
  pushd ~/Downloads
  tar xvf $CMAKE_ARCHIVE
  pushd $CMAKE_FOLDER 
  ./configure
  make
  sudo make install
  popd
  rm $CMAKE_ARCHIVE
  popd
fi

#
# Install FreeRDP dependencies
#
sudo apt-get install -y \
libssl-dev \
libx11-dev libxext-dev libxinerama-dev libxcursor-dev libxkbfile-dev \
libxv-dev libxi-dev libxdamage-dev libxrender-dev libxrandr-dev \
libasound2-dev libcups2-dev libpulse-dev \
libavutil-dev libavcodec-dev \
libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev

#
# Install FreeRDS dependencies
#
sudo apt-get install -y \
libpciaccess-dev libpam0g-dev libpng12-dev libjpeg-dev intltool \
libexpat1-dev libxml-libxml-perl libtool bison flex xsltproc \
libfreetype6-dev libfontconfig1-dev libpixman-1-dev xutils-dev \
protobuf-c-compiler libprotobuf-c0 libprotobuf-c0-dev libboost-dev

#
# Create the installation directory.
#
if [ ! -d $FREERDS_INSTALL_DIR ]; then
  sudo mkdir $FREERDS_INSTALL_DIR
  sudo chmod 777 $FREERDS_INSTALL_DIR
fi

#
# Report the currently installed X server version.
#
X -version

#
# Build X11rdp
#
pushd $GIT_ROOT_DIR/FreeRDP/server/FreeRDS
pushd session-manager/module/X11/service/xorg-build
cmake .
make
cd ..
cp xorg-build/scripts/LocalConfigSettings.cmake .
ln -s xorg-build/external/Source/xorg-server .
popd
popd

#
# Build FreeRDP (with FreeRDS)
#
pushd $GIT_ROOT_DIR/FreeRDP
cmake -DCMAKE_INSTALL_PREFIX=$FREERDS_INSTALL_DIR -DCMAKE_BUILD_TYPE=Debug -DSTATIC_CHANNELS=on -DMONOLITHIC_BUILD=on -DWITH_FDSAPI=off -DWITH_SERVER=on -DWITH_X11RDP=on .
make
#sudo make install
popd
