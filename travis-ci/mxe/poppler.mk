# This file is part of MXE. See LICENSE.md for licensing information.

PKG             := poppler
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 0.48.0
$(PKG)_CHECKSUM := 85a003968074c85d8e13bf320ec47cef647b496b56dcff4c790b34e5482fef93
$(PKG)_SUBDIR   := poppler-$($(PKG)_VERSION)
$(PKG)_FILE     := poppler-$($(PKG)_VERSION).tar.xz
$(PKG)_URL      := https://poppler.freedesktop.org/$($(PKG)_FILE)
#$(PKG)_DEPS     := gcc glib cairo libpng lcms1 jpeg tiff freetype zlib curl qt qt5
$(PKG)_DEPS     := gcc glib cairo libpng lcms jpeg tiff freetype zlib curl qt qt5

define $(PKG)_UPDATE
    $(WGET) -q -O- 'http://poppler.freedesktop.org/' | \
    $(SED) -n 's,.*"poppler-\([0-9.]\+\)\.tar\.xz".*,\1,p' | \
    head -1
endef

define $(PKG)_BUILD
    # Note: Specifying LIBS explicitly is necessary for configure to properly
    #       pick up libtiff (otherwise linking a minimal test program fails not
    #       because libtiff is not found, but because some references are
    #       undefined)
    cd '$(1)' \
        && PATH='$(PREFIX)/$(TARGET)/qt/bin:$(PATH)' \
        ./configure \
        $(MXE_CONFIGURE_OPTS) \
        --disable-silent-rules \
        --enable-xpdf-headers \
        --enable-zlib \
        --enable-cms=lcms2 \
        --enable-libcurl \
        --enable-libtiff \
        --enable-libjpeg \
        --enable-libpng \
        --enable-poppler-glib \
        --enable-poppler-cpp \
        --enable-cairo-output \
        --enable-splash-output \
        --enable-compile-warnings=yes \
        --enable-introspection=auto \
        --disable-libopenjpeg \
        --disable-gtk-test \
        --disable-utils \
        --disable-gtk-doc \
        --disable-gtk-doc-html \
        --disable-gtk-doc-pdf \
        --with-font-configuration=win32 \
        PKG_CONFIG_PATH_$(subst .,_,$(subst -,_,$(TARGET)))='$(PREFIX)/$(TARGET)/qt5/lib/pkgconfig:$(PREFIX)/$(TARGET)/qt5/lib/pkgconfig' \
        CXXFLAGS=-D_WIN32_WINNT=0x0500 \
        LIBTIFF_LIBS="`'$(TARGET)-pkg-config' libtiff-4 --libs`"
    PATH='$(PREFIX)/$(TARGET)/qt/bin:$(PATH)' \
        $(MAKE) -C '$(1)' -j '$(JOBS)' bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS=
    $(MAKE) -C '$(1)' -j 1 install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS=

    # Test program
    '$(TARGET)-g++' \
        -W -Wall -Werror -ansi -pedantic \
        '$(TEST_FILE)' -o '$(PREFIX)/$(TARGET)/bin/test-poppler.exe' \
        `'$(TARGET)-pkg-config' poppler poppler-cpp --cflags --libs`
endef
