[![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/TeXworks/texworks/total?color=0f81c1)](https://github.com/TeXworks/texworks/releases)
[![Packaging status](https://repology.org/badge/tiny-repos/texworks.svg?header=in+repos)](https://repology.org/project/texworks/versions)
[![CD](https://github.com/TeXworks/texworks/actions/workflows/cd.yml/badge.svg)](https://github.com/TeXworks/texworks/actions/workflows/cd.yml)
[![CI](https://github.com/TeXworks/texworks/actions/workflows/ci.yml/badge.svg)](https://github.com/TeXworks/texworks/actions/workflows/ci.yml)
[![Appveyor](https://ci.appveyor.com/api/projects/status/eb4e9108blt0pehr?svg=true)](https://ci.appveyor.com/project/stloeffler/texworks)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/22d3db26f8a542f08d8da056e6779020)](https://www.codacy.com/gh/TeXworks/texworks/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=TeXworks/texworks&amp;utm_campaign=Badge_Grade)

Table Of Contents
-----------------
[About TeXworks](#about-texworks)

[Getting Started](#getting-started)

[Contribution](#contribution)

[Installation](#installation)

[Building TeXworks](#building-texworks)

[License](#license)

About TeXworks
==============

TeXworks is an environment for authoring TeX (LaTeX, ConTeXt, etc) documents,
with a Unicode-based, TeX-aware editor, integrated PDF viewer, and a clean,
simple interface accessible to casual and non-technical users.

TeXworks is inspired by Dick Koch's award-winning TeXShop program for macOS,
which has made quality typesetting through TeX accessible to a wider community
of users, without a technical or intimidating face. The goal of TeXworks is to
deliver a similarly integrated, easy-to-use environment for users on all
platforms, especially GNU/Linux and Windows.

Getting Started
---------------

TeXworks is a free and open-source software project. New users are encouraged to leverage this [guide](https://www.tug.org/texworks/#How_can_you_help), outlining the fundamental process of creating, formatting, and generating PDF documents from within TeXworks.

Contribution
-------------

TeXworks thrives on an active community, and welcoming contributions is paramount. If you encounter bugs or have suggestions for improvement, please share them on our [issues page](https://github.com/TeXworks/texworks/issues). Additionally, you can engage in discussions with other users and developers via our [mailing list](https://tug.org/mailman/listinfo/texworks). Visit the [development page](https://github.com/TeXworks/texworks) to fork a copy of the codebase and start contributing. Visit our [homepage](ps://www.tug.org/texworks/) for more information. If you are new to contributing to open-source projects, we encourage you to visit this [guide](https://opensource.guide/how-to-contribute/) to help you get started.

Installation
------------

Pre-built binaries for Windows, macOS, and Linux are available for immediate download from our official [website](https://www.tug.org/texworks/).

Building TeXworks
-----------------

Notes by Jonathan Kew, updated 2011-03-20, 2015-03-29, 2019-03-21, 2020-06-06 by
Stefan Löffler

To build TeXworks from source, you will need to install

-   CMake <https://cmake.org/>

as well as developer packages (or equivalent) for:

-   Qt <https://www.qt.io/download/>
-   Poppler <https://poppler.freedesktop.org/>
-   Hunspell <https://hunspell.github.io/>

along with their dependencies (such as Freetype, fontconfig, zlib, etc.) If you
also want to build the scripting plugins (optional), you additionally need
development packages for Lua and/or Python. Details will depend on your
platform. On Linux or similar systems, your package manager can probably provide
all these. On the Mac, required libraries can be obtained, e.g., using Homebrew.

Using the latest stable versions of the dependencies is highly recommended,
although TeXworks can be built with versions at least as old as CMake 3.1.0,
Qt 5.2.3, poppler 0.24.5, and hunspell 1.2.9.

Once everything is set up, create a folder for building (e.g., "build") and run
CMake in it to create a Makefile or Xcode project. Finally, run make or use
Xcode to build the application.

Further tips on building TeXworks from source are available on some of the wiki
pages:

-   <https://github.com/TeXworks/texworks/wiki/Building>
-   <https://github.com/TeXworks/texworks/wiki/Building-on-Windows-(MinGW)>
-   <https://github.com/TeXworks/texworks/wiki/Building-on-macOS-(Homebrew)>

License
-------

TeXworks is copyright (C) 2007-2025 by Stefan Löffler, Jonathan Kew, and Charlie
Sharpsteen. Distributed under the terms of the GNU General Public License,
version 2 or (at your option) any later version.
See the file COPYING for details.

The SyncTeX code is copyright (c) 2008-2024 by Jérôme Laurens; see
modules/synctex/synctex_parser.c for license details.
