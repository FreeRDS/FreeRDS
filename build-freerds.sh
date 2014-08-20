#! /bin/bash
#
#===========================================================================
#
# build-freerds.sh
#
# This script builds and installs FreeRDS on a clean Ubuntu or CentOS Linux
# distribution.  Complete the following steps:
#
#   1. Install Ubuntu or CentOS.
#   2. Open a Terminal window.
#   3. Download this script.
#   4. Run the command "chmod +x build-freerds.sh".
#   5. Run the command "./build-freerds.sh
#
# Once the installation has finished, here are instructions for running
# FreeRDS from the command line:
#
#   1. Run the command "cd /opt/FreeRDS/bin".
#   2. Open 2 more Terminal windows as root (e.g., "sudo gnome-terminal &").
#   3. Run "./freerds --nodaemon" in Terminal window 1.
#   4. Run "./freerds-session-manager --nodaemon" in Terminal window 2.
#   5. Connect with FreeRDP using "./xfreerdp /v:localhost /cert-ignore".
#
# Supported platforms include:
#
#   Ubuntu (12.10, 13.x, 14.x)
#   CentOS (6.x)
#
#===========================================================================

GIT_ROOT_DIR=~/git/vworkspace
FREERDP_GIT=https://github.com/vworkspace/FreeRDP.git
FREERDS_GIT=https://github.com/vworkspace/FreeRDS.git
FREERDP_BRANCH=awakecoding
FREERDS_BRANCH=awakecoding
FREERDS_INSTALL_DIR=/opt/FreeRDS

#
# Determine the Linux distro
#
LINUX_DISTRO_NAME=`cat /etc/issue|head -1|awk '{ print $1 }'`

case $LINUX_DISTRO_NAME in
  Ubuntu)
    LINUX_DISTRO_VERSION=`cat /etc/issue|head -1|awk '{ print $2 }'`
    ;;		
  CentOS)
    LINUX_DISTRO_VERSION=`cat /etc/issue|head -1|awk '{ print $3 }'`
    ;;
  *)
    echo "Unsupported Linux distro '$LINUX_DISTRO_NAME'"
    exit
    ;;
esac

#
# Fetch sources from GitHub
#
case $LINUX_DISTRO_NAME in
  Ubuntu)
    sudo apt-get install -y git-core
    ;;
  CentOS)
    sudo yum install -y git-core
    ;;
esac

if [ ! -d $GIT_ROOT_DIR ]; then
  mkdir -p $GIT_ROOT_DIR
fi

if [ ! -d $GIT_ROOT_DIR/FreeRDP ]; then
  pushd $GIT_ROOT_DIR
  git clone $FREERDP_GIT
  cd FreeRDP
  git checkout $FREERDP_BRANCH
  popd
fi

if [ ! -d $GIT_ROOT_DIR/FreeRDP/server/FreeRDS ]; then
  pushd $GIT_ROOT_DIR/FreeRDP/server
  git clone $FREERDS_GIT
  cd FreeRDS
  git checkout $FREERDS_BRANCH
  popd
fi

#
# On 64-bit platforms, point to shared objects.
#
export LD_LIBRARY_PATH=/usr/lib

#
# Install the correct version of CMake
#
case $LINUX_DISTRO_NAME in
  Ubuntu)
    sudo apt-get install -y build-essential
    ;;
  CentOS)
    sudo yum groupinstall -y "Development Tools"
    ;;
esac

CMAKE_VERSION=2.8.12
CMAKE_FOLDER=cmake-$CMAKE_VERSION
CMAKE_ARCHIVE=$CMAKE_FOLDER.tar.gz
CMAKE_URL=http://www.cmake.org/files/v2.8/$CMAKE_ARCHIVE

RESULT=`cmake --version`
if [ "$RESULT" == "" ]; then
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
else
  echo Found $RESULT
fi

#
# Install FreeRDP dependencies
#
case $LINUX_DISTRO_NAME in
  Ubuntu)
    sudo apt-get install -y \
    libssl-dev \
    libx11-dev libxext-dev libxinerama-dev libxcursor-dev libxkbfile-dev \
    libxv-dev libxi-dev libxdamage-dev libxrender-dev libxrandr-dev \
    libasound2-dev libcups2-dev libpulse-dev \
    libavutil-dev libavcodec-dev \
    libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev
	;;

  CentOS)
    sudo yum install -y \
    openssl-devel \
    libX11-devel libXext-devel libXinerama-devel libXcursor-devel libxkbfile-devel \
    libXv-devel libXtst-devel libXi-devel libXdamage-devel libXrandr-devel \
    alsa-lib-devel cups-devel ffmpeg-devel glib2-devel
    ;;
esac

#
# Install FreeRDS dependencies
#
case $LINUX_DISTRO_NAME in
  Ubuntu)
    sudo apt-get install -y \
    libpciaccess-dev libpam0g-dev libpng12-dev libjpeg-dev intltool \
    libexpat1-dev libxml-libxml-perl libtool bison flex xsltproc \
    libfreetype6-dev libfontconfig1-dev libpixman-1-dev xutils-dev \
    x11proto-gl-dev mesa-common-dev libgl1-mesa-dev xorg-dev \
    protobuf-c-compiler libprotobuf-c0 libprotobuf-dev libprotobuf-c0-dev \
    libboost-dev qt4-dev-tools
    ;;

  CentOS)
    sudo yum install -y \
    finger patch gcc gcc-c++ make autoconf libtool automake pkgconfig \
    libpciaccess-devel openssl-devel gettext file pam-devel libjpeg-devel pixman-devel \
    libX11-devel libXfixes-devel libXfont-devel xorg-x11-proto-devel xorg-x11-xtrans-devel \
    flex bison libxslt perl-libxml-perl xorg-x11-font-utils xmlto-tex docbook-utils-pdf \
    boost-devel qt4-devel
    ;;
esac

#
# Install the correct version of Apache Thrift
#
THRIFT_VERSION=0.9.0
THRIFT_FOLDER=thrift-$THRIFT_VERSION
THRIFT_ARCHIVE=$THRIFT_FOLDER.tar.gz
THRIFT_URL=http://archive.apache.org/dist/thrift/$THRIFT_VERSION/$THRIFT_ARCHIVE

RESULT=`thrift --version`
if [ "$RESULT" == "" ]; then
  # Download compressed archive
  pushd ~/Downloads
  wget $THRIFT_URL
  popd

  # Unpack compressed archive
  pushd ~/Downloads
  tar xvf $THRIFT_ARCHIVE
  pushd $THRIFT_FOLDER
  ./configure --without-python --without-java --without-c_glib --with-pic --without-csharp --without-haskell --without-go --without-d --without-qt4
  make
  sudo make install
  popd
  rm $THRIFT_ARCHIVE
  popd
else
  echo Found $RESULT
fi

#
# Install the correct version of Google protocol buffers
#
PROTOBUF_VERSION=2.4.1
PROTOBUF_FOLDER=protobuf-$PROTOBUF_VERSION
PROTOBUF_ARCHIVE=$PROTOBUF_FOLDER.tar.gz
PROTOBUF_URL=https://protobuf.googlecode.com/files/$PROTOBUF_ARCHIVE

RESULT=`protoc --version`
if [ "$RESULT" == "" ]; then
  # Download compressed archive
  pushd ~/Downloads
  wget $PROTOBUF_URL
  popd

  # Unpack compressed archive
  pushd ~/Downloads
  tar xvf $PROTOBUF_ARCHIVE
  pushd $PROTOBUF_FOLDER
  ./configure --prefix=/usr
  make
  sudo make install
  popd
  rm $PROTOBUF_ARCHIVE
  popd
else
  echo Found $RESULT
fi

PROTOBUFC_VERSION=0.15
PROTOBUFC_FOLDER=protobuf-c-$PROTOBUFC_VERSION
PROTOBUFC_ARCHIVE=$PROTOBUFC_FOLDER.tar.gz
PROTOBUFC_URL=https://protobuf-c.googlecode.com/files/$PROTOBUFC_ARCHIVE

RESULT=`protoc-c --version`
if [ "$RESULT" == "" ]; then
  # Download compressed archive
  pushd ~/Downloads
  wget $PROTOBUFC_URL
  popd

  # Unpack compressed archive
  pushd ~/Downloads
  tar xvf $PROTOBUFC_ARCHIVE
  pushd $PROTOBUFC_FOLDER
  ./configure --prefix=/usr
  make
  sudo make install
  popd
  rm $PROTOBUFC_ARCHIVE
  popd
else
  echo Found $RESULT
fi

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
ln -s xorg-build/external/Source/xorg-server .
popd
popd

#
# Build FreeRDP (with FreeRDS)
#
pushd $GIT_ROOT_DIR/FreeRDP
cmake -DCMAKE_INSTALL_PREFIX=$FREERDS_INSTALL_DIR -DCMAKE_BUILD_TYPE=Debug -DSTATIC_CHANNELS=on -DMONOLITHIC_BUILD=on -DWITH_FDSAPI=on -DWITH_SERVER=on -DWITH_X11RDP=on .
make
sudo make install
popd
