#!/bin/sh
# Set to a stable release version of libiec61850
libiec61850_VSN="v1.5.3"

set -e

# the script folder
DIR=$PWD
BASEDIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# dive into c_src
cd $BASEDIR

# detecting gmake and if exists use it
# if not use make
which gmake 1>/dev/null 2>/dev/null && MAKE=gmake
MAKE=${MAKE:-make}

# This is a workaround of compilation error of cJSON: 
# generic selections are a C11-specific feature
TARGET_OS=`uname -s`
if [ "$TARGET_OS" = "Darwin" ] || [ "$TARGET_OS" = "FreeBSD" ]; then
    export CFLAGS="$CFLAGS -Wno-c11-extensions"
fi

case "$1" in
    clean)
        rm -rf libiec61850
        ;;

    build)
        # libiec61850
        if [ ! -d libiec61850 ]; then
            git clone --depth 1 -b $libiec61850_VSN https://github.com/mz-automation/libiec61850.git
        fi
        cd libiec61850
        mkdir -p build
        cd build
        
        # Compile and install locally to _install folder
        cmake -DCMAKE_INSTALL_PREFIX=_install .. 
        $MAKE && $MAKE install
        ;;
esac

cd $DIR
