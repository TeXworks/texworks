[![CD](https://github.com/TeXworks/texworks/workflows/CD/badge.svg)](https://github.com/TeXworks/texworks/actions?query=workflow%3ACD)
[![CI](https://github.com/TeXworks/texworks/workflows/CI/badge.svg)](https://github.com/TeXworks/texworks/actions?query=workflow%3ACI)
[![Appveyor](https://ci.appveyor.com/api/projects/status/eb4e9108blt0pehr?svg=true)](https://ci.appveyor.com/project/stloeffler/texworks)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/22d3db26f8a542f08d8da056e6779020)](https://www.codacy.com/gh/TeXworks/texworks/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=TeXworks/texworks&amp;utm_campaign=Badge_Grade)

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

Further Information
-------------------

If you find any bugs/problems or have any recommendations, don't hesitate to
stop by the development webpage, send a mail to the mailing list (preferably via
the "Help > Email to mailing list" menu item which automatically includes some
debug information), or file a bug report.

-   Homepage:     <https://www.tug.org/texworks/>
-   Development:  <https://github.com/TeXworks/texworks>
-   Bugs:         <https://github.com/TeXworks/texworks/issues>
-   Mailing list: <https://tug.org/mailman/listinfo/texworks>

License
-------

TeXworks is copyright (C) 2007-2023 by Jonathan Kew, Stefan Löffler, and Charlie
Sharpsteen. Distributed under the terms of the GNU General Public License,
version 2 or (at your option) any later version.
See the file COPYING for details.

The SyncTeX code is copyright (c) 2008-2017 by Jérôme Laurens; see
modules/synctex/synctex_parser.c for license details.

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
