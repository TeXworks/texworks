

_If you have corrections, additions, or updates to the instructions given below - or instructions for another distribution not covered yet -, please contact us (preferably on the [mailing list](http://tug.org/mailman/listinfo/texworks) or via email._

Building TeXworks on typical GNU/Linux systems is fairly straightforward. You just need standard build tools (gcc, make, cmake, etc.), and the development files (not just runtime libraries) for Qt4, poppler, hunspell, and dbus. The exact set of packages needed will depend how your distribution arranges things; some examples are given here.

For Windows users wanting to try a build, see the page about installing and [using the MinGW tools](BuildingOnWindowsMinGW.md). Thanks to Alain Delmotte and Tomek for their help in researching, testing, and documenting the Windows build procedure.

For Mac users, see the page about [building on Mac OS X using Homebrew](BuildingOnMacOSXHomebrew.md). Thanks to Charlie Sharpsteen for researching and documenting this Mac OS X build procedure.

If you not only want to build TeXworks, but also package it to share with others, have a look at the file [PACKAGING](http://code.google.com/p/texworks/source/browse/trunk/PACKAGING) in the source tree.

# Fedora #
## Fedora 9 ##

```
# yum install poppler-devel qt4-devel hunspell-devel
$ svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only
$ cd texworks-read-only
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./texworks
```

(Thanks to Dave Crossland for this.)

## Fedora 10 ##

```
$ sudo yum groupinstall x-software-development development-tools
$ sudo yum install subversion poppler-devel poppler-qt4-devel qt-devel hunspell-devel dbus-devel
$ svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only
$ cd texworks-read-only
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./texworks
```

(Thanks to Dave Crossland for this.)

# Ubuntu & Debian #

```
$ sudo aptitude install build-essential subversion libpoppler-qt4-dev libhunspell-dev libdbus-1-dev liblua5.1-0-dev zlib1g-dev
$ svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only
$ cd texworks-read-only
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./texworks
```

Another option is to use the [Ubuntu package repository](https://launchpad.net/~texworks/+archive/ppa), thus avoiding the need to compile from source.

# OpenSUSE #

Adapted from a report by [msiniscalchi](http://code.google.com/u/msiniscalchi/):

Under OpenSUSE 11.1, I had to install the following devel packages:

```
gcc
make
cmake
hunspell-devel
poppler-devel
libqt4-devel
libpoppler-qt4-devel
```

Running `cmake` and then `make` produces the binary.

# Slackware #

Adapted from a report by wtx358:

Install the following packages:
```
qt
hunspell
poppler
poppler-data
dbus
```

Then run
```
svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only
cd texworks-read-only
mkdir build
cd build
cmake ..
make
./texworks
```