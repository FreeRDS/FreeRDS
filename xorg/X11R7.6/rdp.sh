#!/bin/sh

PREFIX_DIR=/opt/X11rdp

echo "using $PREFIX_DIR"

export PKG_CONFIG_PATH=$PREFIX_DIR/lib/pkgconfig:$PREFIX_DIR/share/pkgconfig
export PATH=$PREFIX_DIR/bin:$PATH
export LDFLAGS=-Wl,-rpath=$PREFIX_DIR/lib
export CFLAGS="-I$PREFIX_DIR/include -fPIC -O2"

X11RDPBASE=$PREFIX_DIR
export X11RDPBASE

cd rdp
make
if [ $? -ne 0 ]; then
    echo "error building rdp"
    exit 1
fi

strip X11rdp
cp X11rdp $X11RDPBASE/bin

echo "All done"
