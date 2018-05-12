# This file contains a formula for installing Poppler on Mac OS X using the
# Homebrew package manager:
#
#     http://brew.sh/
#
# To install Poppler using this formula:
#
#     brew install path/to/this/poppler.rb
#
# Changes compared to Homebrew's standard Poppler formula:
#
#   - TeXworks-specific patches are applied to
#        - help Qt apps find the poppler-data directory.
#        - use native Mac OS X font handling (instead of fontconfig)
class Poppler < Formula
  desc "PDF rendering library (based on the xpdf-3.0 code base)"
  homepage "https://poppler.freedesktop.org/"
  url "https://poppler.freedesktop.org/poppler-0.64.0.tar.xz"
  sha256 "b21df92ca99f78067785cf2dc8e06deb04726b62389c0ee1f5d8b103c77f64b1"
  head "https://anongit.freedesktop.org/git/poppler/poppler.git"

# BEGIN TEXWORKS MODIFICATION
#  bottle do
#    sha256 "fae8e28f893316a98df04746bd437233c5a8cd63df27ffee5fb82f79bd139d77" => :high_sierra
#    sha256 "14430fd4f774cd862215b91c9e9a4801a064c943638a576038c139fd617495d9" => :sierra
#    sha256 "b24d5a380d95dc37576bb7929906d54704cc58270f66100cd783cfe0b695ae00" => :el_capitan
#  end

  version '0.64.0-texworks'

  TEXWORKS_SOURCE_DIR = Pathname.new(__FILE__).realpath.dirname.join('../../..')
  TEXWORKS_PATCH_DIR = TEXWORKS_SOURCE_DIR + 'lib-patches/'
  patch do
    url "file://" + TEXWORKS_PATCH_DIR + 'poppler-0001-Fix-bogus-memory-allocation-in-SplashFTFont-makeGlyp.patch'
    sha256 "82b6c00194259245a1a3cc451e5026e0f838ba18131dd530e98cae14978cb342"
  end
  patch do
    url "file://" + TEXWORKS_PATCH_DIR + 'poppler-0002-Native-Mac-font-handling.patch'
    sha256 "3920f5fd1ecb1d810cd3f422bd454d347fc05428c853bcae29a3312be2f2eebd"
  end
  patch do
    url "file://" + TEXWORKS_PATCH_DIR + 'poppler-0003-Add-support-for-persistent-GlobalParams.patch'
    sha256 "354af3e87f575a16c6eb00cf545049d56a4f6bda3828801c55a2d568fa190f44"
  end
# END TEXWORKS MODIFICATION

  option "with-qt", "Build Qt5 backend"
  option "with-little-cms2", "Use color management system"
  option "with-nss", "Use NSS library for PDF signature validation"

  deprecated_option "with-qt4" => "with-qt"
  deprecated_option "with-qt5" => "with-qt"
  deprecated_option "with-lcms2" => "with-little-cms2"

  depends_on "cmake" => :build
  depends_on "gobject-introspection" => :build
  depends_on "pkg-config" => :build
  depends_on "cairo"
  depends_on "fontconfig"
  depends_on "freetype"
  depends_on "gettext"
  depends_on "glib"
  depends_on "jpeg"
  depends_on "libpng"
  depends_on "libtiff"
  depends_on "openjpeg"
  depends_on "qt" => :optional
  depends_on "little-cms2" => :optional
  depends_on "nss" => :optional

  conflicts_with "pdftohtml", "pdf2image", "xpdf",
    :because => "poppler, pdftohtml, pdf2image, and xpdf install conflicting executables"

  resource "font-data" do
    url "https://poppler.freedesktop.org/poppler-data-0.4.9.tar.gz"
    sha256 "1f9c7e7de9ecd0db6ab287349e31bf815ca108a5a175cf906a90163bdbe32012"
  end

  needs :cxx11 if build.with?("qt") || MacOS.version < :mavericks

  def install
    # Remove for > 0.64.0
    # Fix "error: implicit instantiation of undefined template 'std::__1::array<double, 257>'"
    # Upstream issue from 18 Apr 2018 "0.64.0 build with qt5 fails"
    # See https://bugs.freedesktop.org/show_bug.cgi?id=106118
    if build.with? "qt"
      inreplace "qt5/src/ArthurOutputDev.cc", /#include <QPicture>/,
                                              "\\0\n#include <array>"
    end

    ENV.cxx11 if build.with?("qt") || MacOS.version < :mavericks

    args = std_cmake_args + %w[
      -DENABLE_XPDF_HEADERS=ON
      -DENABLE_GLIB=ON
      -DBUILD_GTK_TESTS=OFF
      -DWITH_GObjectIntrospection=ON
      -DENABLE_QT4=OFF
    ]

    if build.with? "qt"
      args << "-DENABLE_QT5=ON"
    else
      args << "-DENABLE_QT5=OFF"
    end

    if build.with? "little-cms2"
      args << "-DENABLE_CMS=lcms2"
    else
      args << "-DENABLE_CMS=none"
    end

    system "cmake", ".", *args
    system "make", "install"
    system "make", "clean"
    system "cmake", ".", "-DBUILD_SHARED_LIBS=OFF", *args
    system "make"
    lib.install "libpoppler.a"
    lib.install "cpp/libpoppler-cpp.a"
    lib.install "glib/libpoppler-glib.a"
    resource("font-data").stage do
      system "make", "install", "prefix=#{prefix}"
    end

    libpoppler = (lib/"libpoppler.dylib").readlink
    ["#{lib}/libpoppler-cpp.dylib", "#{lib}/libpoppler-glib.dylib",
     *Dir["#{bin}/*"]].each do |f|
      macho = MachO.open(f)
      macho.change_dylib("@rpath/#{libpoppler}", "#{lib}/#{libpoppler}")
      macho.write!
    end
  end

  test do
    system "#{bin}/pdfinfo", test_fixtures("test.pdf")
  end
end
