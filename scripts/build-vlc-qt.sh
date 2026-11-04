#!/usr/bin/env bash
set -euo pipefail

root=$(cygpath -u "$VLCPLUS_ROOT")
source_parent="$root/build/vlc-source"
source="$source_parent/vlc-3.0.20"
build="$root/build/vlc-native"
archive="$root/build/downloads/vlc-3.0.20.tar.xz"
jobs=${VLCPLUS_JOBS:-2}
mkdir -p "$source_parent" "$build"

# Restore the pinned upstream inputs in the generated build tree before patching.
tar -xJf "$archive" -C "$source_parent"
git -C "$source" init -q
git -C "$source" apply --check "$root/vlc-qt/vlc-3.0.20-flubber.patch"
git -C "$source" apply "$root/vlc-qt/vlc-3.0.20-flubber.patch"
cp "$root/vlc-qt/flubber_panel.hpp" "$root/vlc-qt/flubber_bridge.hpp" "$source/modules/gui/qt/"

qt_version=$(qmake-qt5 -query QT_VERSION)
cd "$build"
if [[ ! -f .configured ]]; then
    BUILDCC=gcc "$source/configure" --host=x86_64-w64-mingw32 \
        --disable-dependency-tracking --disable-lua --disable-skins2 --disable-nls --disable-update-check \
        --disable-avcodec --disable-avformat --disable-swscale --disable-a52 --disable-mad
    touch .configured
fi

make -j"$jobs" -C compat CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types'
make -j"$jobs" -C src CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types'
make -j"$jobs" -C lib libvlc.la CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types'
make -j"$jobs" -C modules -f - vlc-plus-qt-generated <<'MAKE'
include Makefile
vlc-plus-qt-generated: $(nodist_libqt_plugin_la_SOURCES) module.rc.lo
MAKE
make -j"$jobs" -C modules libqt_plugin.la \
    am__append_4=../modules/module.rc.lo LIBS_qt=-lwinmm \
    CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types' \
    CXXFLAGS="-g -O2 -I/mingw64/include/QtGui/$qt_version/QtGui -I/mingw64/include/QtGui/$qt_version"
make -j"$jobs" -C bin vlc.exe CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types'
strip --strip-unneeded modules/.libs/libqt_plugin.dll bin/.libs/vlc.exe
pacman -Q > "$root/build/toolchain-packages.txt"
