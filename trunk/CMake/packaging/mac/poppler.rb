# This file contains a formula for installing Poppler on Mac OS X using the
# Homebrew package manager:
#
#     http://mxcl.github.com/homebrew
#
# To install Poppler using this formula:
#
#     brew install path/to/this/poppler.rb
#
# Changes compared to Homebrew's standard Poppler formula:
#
#   - TeXworks-specific patches are applied to help Qt apps find the
#     poppler-data directory.
#
#   - Poppler is configured to use as few dependencies as possible. This
#     reduces the number of dylibs that must be added to TeXworks.app when it
#     is packaged for distribution.
TEXWORKS_SOURCE_DIR = Pathname.new(__FILE__).realpath.dirname.join('../../..')
TEXWORKS_PATCH_DIR = TEXWORKS_SOURCE_DIR + 'lib-patches'

require 'formula'

class PopplerData < Formula
  url 'http://poppler.freedesktop.org/poppler-data-0.4.5.tar.gz'
  md5 '448dd7c5077570e340340706cef931aa'
end

class Poppler < Formula
  url 'http://poppler.freedesktop.org/poppler-0.18.1.tar.gz'
  homepage 'http://poppler.freedesktop.org'
  md5 'd30e883a27423c936ef338ce1d967e2d'
  version '0.18.1-texworks'

  depends_on 'pkg-config' => :build
  depends_on 'qt'

  def patches
    {
      :p1 => [
        TEXWORKS_PATCH_DIR + 'poppler-qt4-globalparams.patch',
        TEXWORKS_PATCH_DIR + 'poppler-bogus-memory-allocation-fix.patch',
        TEXWORKS_PATCH_DIR + 'poppler-fix-cmake-install-names-for-homebrew.patch'
      ]
    }
  end

  def install
    ENV.x11
    cmake_args = std_cmake_parameters.split

    # Save time by not building tests
    cmake_args.concat [
      '-DBUILD_CPP_TESTS=OFF',
      '-DBUILD_GTK_TESTS=OFF',
      '-DBUILD_QT3_TESTS=OFF',
      '-DBUILD_QT4_TESTS=OFF'
    ]

    # Components required by TeXworks.
    cmake_args.concat [
      '-DWITH_Qt4=YES',
      '-DENABLE_XPDF_HEADERS=YES'
    ]

    # Minimize library dependencies for TeXworks
    cmake_args.concat [
      '-DENABLE_ABIWORD=OFF',
      '-DENABLE_CPP=OFF',
      '-DENABLE_LCMS=OFF',
      '-DENABLE_LIBCURL=OFF',
      '-DENABLE_LIBOPENJPEG=OFF',
      '-DENABLE_SPLASH=ON', # Required
      '-DENABLE_UTILS=OFF',
      '-DENABLE_ZLIB=OFF',
      '-DWITH_Cairo=OFF',
      '-DWITH_JPEG=OFF',
      '-DWITH_PNG=OFF',
      '-DWITH_Qt3=OFF'
    ]

    Dir.mkdir 'build'
    Dir.chdir 'build' do
      system 'cmake', '..', *cmake_args
      system "make install"
    end

    # Install poppler font data.
    PopplerData.new.brew do
      system "make install prefix=#{prefix}"
    end
  end
end
