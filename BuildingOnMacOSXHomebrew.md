_Thanks to Charlie Sharpsteen for developing this guide and providing the files._

**Note**: This guide is still work-in-progress. If you find something is not working on your system, please drop us a mail on the [mailing list](http://tug.org/mailman/listinfo/texworks).



# Preface #

This document describes a procedure for building TeXworks on Mac OS X 10.6 using [Homebrew](http://mxcl.github.com/homebrew/) <http://mxcl.github.com/homebrew/> and CMake. It is by no means the only way possible and it doesn't anticipate and tackle all possible problems, either. If you run into any difficulties feel free to ask for help on the [mailing list](http://tug.org/mailman/listinfo/texworks) <http://tug.org/mailman/listinfo/texworks>.

Note that compiling is usually done in the terminal. This may be unfamiliar to many Mac users, but the terminal is a powerful, very versatile, and commonly used tool for software development. Don't worry, however, as this guide will walk you through each step.

# Prerequisites #

The basic requirements are Homebrew and Xcode. For a more in-depth guide, see <https://github.com/mxcl/homebrew/wiki/Installation>.

If you have some of the prerequisites installed already, you can skip the respective sections, of course.

## Xcode ##
Xcode is Apple's integrated development environment that provides (among other things) the compiler necessary to build libraries and TeXworks itself. It is available at http://developer.apple.com/xcode/. If you are not part of the iOS or Mac Developer Programs, be sure to look for "Xcode 3" at the bottom of the page, which only requires you to register, but is free of charge.

## Homebrew ##
Homebrew claims to be "the easiest and most flexible way to install the UNIX tools Apple didn't include with OS X". In this guide, it is used to get the libraries necessary to build TeXworks.

The easiest (and [recommended](https://github.com/mxcl/homebrew/wiki/Installation)) way to install Homebrew is to run the following command in the terminal:
```
ruby -e "$(curl -fsSLk https://gist.github.com/raw/323731/install_homebrew.rb)"
```
which will download and run the installation script.

## Obtaining necessary tools and libraries ##
For building TeXworks, you need several tools and libraries, namely:
  * git: used to keep track of the TeXworks sources and build scripts
  * pkg-config: is used by the build scripts to find libraries
  * CMake: interprets and executes the build scripts
  * Qt: the toolkit which is used to build the graphical user interface
  * hunspell: a spellchecking library
  * poppler: a library for displaying PDF files

To install these, run the following commands in a terminal:
```
brew install git
brew install pkg-config
brew install cmake
brew install qt
brew install hunspell
brew install poppler
```
_Warning: Some of these will take a long time to finish. Especially Qt is quite large (several 100s of MB), so make sure you have a good internet connection and (up to) a couple of hours. The good thing is that each installation should run without requiring any input from you._

If you want to build TeXworks with Lua scripting support, you also need to run
```
brew install lua
```

# Obtaining and building TeXworks #

To obtain the TeXworks sources themselves and the corresponding build scripts, run
```
git clone git://github.com/Sharpie/TeXworks.git
```
This will download the required files and put them into a folder named `TeXworks` that is created in the current folder.

Next, you need to create a folder named `build` inside `TeXworks`. To do that, run the following commands:
```
cd TeXworks
mkdir build
```

To finally build the program, navigate to the newly created `build` directory (e.g., by running `cd build`) and there run
```
cmake ..
make
```

If you want to install TeXworks, you can use the following command:
```
make install
```

# Updating TeXworks #
To update TeXworks, again navigate to the `build` directory and run the following commands:
```
git pull
cmake ..
make
```
and possibly
```
make install
```

From time to time, you should also check if some of you Homebrew packages are out-of-date. To do that, run the following commands:
```
brew update
brew outdated
```
