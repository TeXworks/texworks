
---

**This guide is outdated, please use [BuildingOnWindowsMinGW](BuildingOnWindowsMinGW.md) instead. If that is not an option for you, please write to the mailing list for further help.**

---

**Note:** An open source alternative to this guide is to use the gcc-based MinGW toolset; see the [BuildingOnWindowsMinGW](BuildingOnWindowsMinGW.md) page for details.

**Note:** At the moment, KDE on Windows seems to distribute only 32 bit versions of the libraries. If you are running a 64 bit system, this guide probably won't work.

_Thanks to Stefan Löffler and Alain Delmotte for developing this guide._



# Preface #

This document describes a procedure for building TeXworks on Windows XP using the (freely available) Microsoft Visual C++ 2008 Express Edition. It is by no means the only way possible and it doesn't anticipate and tackle all possible problems, either. If you run into any difficulties feel free to ask for help on the [mailing list](http://tug.org/mailman/listinfo/texworks).

_Note: This guide is designed for use with the English version on Windows XP. If you use another language the labels and some of the paths will be different._

# Required Software #

The process described here has been sucessfully implemented using the following software. You may of course use other software, but then the described steps may have to be adapted in some places.
  * Windows XP (+SP3; Vista should work, too, but wasn't tested yet)
  * [Visual C++ 2008 Express Edition (+SP1)](http://www.microsoft.com/express/vc/)<br>for building some required libraries and the program itself. For brevity this application will be referred to as MSVC in the rest of this document.<br>Note: This software is for free (as in "beer", not as in "speech"). To set it up properly first follow the <a href='http://techbase.kde.org/Getting_Started/Build/KDE4/Windows/MS_Visual_Studio#Visual_Studio_2008_SP1_Express_Edition'>guide from the KDE on Windows project</a>.<br>
<ul><li><a href='http://www.7-zip.org/'>7-Zip</a><br>for opening tar.gz<br>Note: This software is for free (as in "beer", mostly also as in "speech")<br>
</li><li><a href='http://subversion.tigris.org/'>Subversion</a><br>for obtaining the TeXworks source code<br>Note: This software is for free (as in "speech")<br>
<i>Note: You need administrator's privileges to install new software.</i></li></ul>

<h1>Directory structure</h1>

The following directory layout is used in this guide. If you choose another layout you have to adjust the paths in the rest of this guide.<br>
<pre><code>C:\texworks\hunspell-1.2.8\ (hunspell)<br>
C:\texworks\texworks-read-only\ (TeXworks)<br>
</code></pre>

<h1>Obtaining the necessary libraries from KDE on Windows</h1>

TeXworks depends on several external libraries, most notably Qt and Poppler. You can obtain most of the needed dependencies from <a href='http://windows.kde.org/'>the KDE on Windows project</a>. See their <a href='http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps'>summary of steps</a>
for further information.<br>
<br>
Download and run the installer. Choose the following options when asked:<br>
<ul><li>Installation Directory: Choose a path <b>not containing spaces or special characters</b> (this includes accented characters, umlauts, etc.; only use <a href='http://en.wikipedia.org/wiki/ASCII'>ASCII</a>), e.g. C:\KDE<br>
</li><li>Install Mode: Package Manager<br>
</li><li>Compiler Mode: MSVC 32Bit<br>
<i>Note: You need administrator's privileges to install some of the packages.</i></li></ul>

Install the following packages and their dependencies (make sure that you<br>
install both the <b>"bin" and the "devel"</b> packages where available):<br>
<ul><li>jpeg,<br>
</li><li>poppler-data,<br>
</li><li>poppler-vc90,<br>
</li><li>qt-vc90,<br>
</li><li>zlib<br>
<i>Note: Not all available servers provide all necessary packages. If the one you chose doesn't, choose another.</i></li></ul>

Add the KDEDIRS environment variable and adjust your path to include the KDE bin and lib directories according to the <a href='http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps'>summary of steps required for installation of KDE on Windows</a>

<h1>Downloading and building Hunspell</h1>

The hunspell library is required for spell checking and is not bundled with KDE on Windows. Hence you have to compile it yourself.<br>
<br>
Go to <a href='http://sourceforge.net/projects/hunspell/'>http://sourceforge.net/projects/hunspell/</a> and select "View all files".<br>
<br>
Download the .tar.gz file (the win32.zip file only contains programs, no sources and no libraries).<br>
<br>
At the time of writing, hunspell-1.2.8.tar.gz was the latest version and will be used throughout this guide.<br>
<br>
Extract hunspell-1.2.8.tar.gz to C:\texworks\<br>
<br>
Open hunspell-1.2.8/src/win_api/Hunspell.sln (the solution needs to be converted)<br>
<br>
Two files are missing in the libhunspell project. Follow the next steps to add them:<br>
<ol><li>Expand the libhunspell project in the solution explorer on the left hand side<br>
</li><li>Right-Click on the hunspell folder under the libhunspell project<br>
</li><li>Choose Add->Existing Item<br>
</li><li>Choose C:\texworks\hunspell-1.2.8\src\hunspell\replist.<code>*</code></li></ol>

Since all other libraries are built in Release mode, hunspell should be built in that mode as well. To switch to that mode, choose Configuration Manager from the Build menu. In the popup window, choose Release_dll as the active configuration for libhunspell and close the window.<br>
<br>
Build libhunspell (i.e. by Right-Clicking on libhunspell in the solution explorer and choosing "Build")<br>
<br>
There are several warnings but the build should succeed.<br>
<br>
<h1>Obtaining and building TeXworks</h1>

Run the following command from the command line in the folder C:\texworks<br>
<pre><code>svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only<br>
</code></pre>

You get a fresh new copy of the latest TeXworks code in the folder C:\texworks\texworks-read-only<br>
<br>
In order to build TeXworks you have to modify C:\texworks\texworks-read-only\TeXworks.pro to reflect your local configuration. Find the lines reading<br>
<pre><code>	INCLUDEPATH += c:/MinGW514/local/include<br>
	INCLUDEPATH += c:/MinGW514/local/include/poppler<br>
	INCLUDEPATH += c:/MinGW514/local/include/poppler/qt4<br>
	INCLUDEPATH += c:/MinGW514/local/include/hunspell<br>
<br>
	LIBS += -Lc:/MinGW514/local/lib<br>
	LIBS += -lpoppler-qt4<br>
	LIBS += -lpoppler<br>
	LIBS += -lfreetype<br>
	LIBS += -lhunspell-1.2<br>
	LIBS += -lz<br>
	LIBS += -lgdi32<br>
</code></pre>
and replace them by<br>
<pre><code>	INCLUDEPATH += C:/KDE/include/poppler/qt4<br>
	INCLUDEPATH += C:/texworks/hunspell-1.2.8/src/hunspell<br>
<br>
	LIBS += C:/KDE/lib/poppler-qt4.lib<br>
	LIBS += C:/KDE/lib/poppler.lib<br>
	LIBS += C:/texworks/hunspell-1.2.8/src/win_api/Release_dll/libhunspell/libhunspell.lib<br>
	LIBS += C:/KDE/lib/zlib.lib<br>
</code></pre>
In addition, unless you are using a poppler installation that includes the xpdf compatibility headers (not provided by default poppler builds, this requires a configure option), you will need to find the line<br>
<pre><code>QMAKE_CXXFLAGS += -DHAVE_POPPLER_XPDF_HEADERS<br>
</code></pre>
and comment it out by inserting # at the beginning. The result will be that TeXworks will not automatically locate a poppler-data directory alongside the executable, but will need the poppler data files (for CJK font support) installed in a fixed system location that is dependent on the poppler library build.<br>
<br>
Then run<br>
<pre><code>qmake -tp vc<br>
</code></pre>
on the command line from the C:\texworks\texworks-read-only directory. This creates C:\texworks\texworks-read-only\TeXworks.vcproj which can be opened in MSVC.<br>
<br>
Choose the Release configuration in the same way as for hunspell (Note: it's simply called Release this time).<br>
<br>
Build TeXworks (i.e. by Right-Clicking on the TeXworks project in the solution explorer and choosing "Build"). This creates C:\texworks\texworks-read-only\release\TeXworks.exe<br>
<br>
<i>Note: When asked whether to save the solution it doesn't matter what you choose.</i>

<h1>Running TeXworks for the first time</h1>

Before you can run TeXworks for the first time you need to copy the hunspell dll into a location where TeXworks can find them. Copy C:\texworks\hunspell-1.2.8\src\win_api\Release_dll\libhunspell\libhunspell.dll to C:\texworks\texworks-read-only\release.<br>
<br>
<h1>Updating TeXworks</h1>

Remove the following files (if they exist) from C:\texworks\texworks-read-only:<br>
<ul><li>TeXworks.sln<br>
</li><li>TeXworks.ncb<br>
</li><li>TeXworks.vcproj<br>
</li><li>TeXworks.suo</li></ul>

Then run<br>
<pre><code>svn update<br>
qmake -tp vc<br>
</code></pre>
from the same directory to update your TeXworks sources and your MSVC project file. Finally open TeXworks.vcproj in MSVC, make sure the Release configuration is selected and build.<br>
<br>
<i>Note: When asked whether to save the solution it doesn't matter what you choose.</i>

<h1>Disclaimer</h1>
This guide is provided as-is in the hope that it's helpful. There is no guarantee the described procedure will work on your system. Use at your own risk.<br>
<br>
<h1>Q & A</h1>

I already have the version X of the software/package Y installed. Do I really have to install it again as described in this guide?<br>
<blockquote>You don't necessarily have to. The full version of MSVC shouldn't be a problem. Other versions will, however (see below). Different versions of the KDE libraries could be a problem as well. This is especially true for different versions of Qt. All the packages of the KDE on Windows project fit together (are compiled by similar compilers and similar configurations, etc.). Mixing versions will most likely cause problems.</blockquote>

I really want to install the KDE on Windows packages into a folder containing spaces. Can't it be done?<br>
<blockquote>Yes, you can try. If everything works well and all settings are correct it should work on most systems. Some of the workarounds presented here won't work, however. So to be on the safe side installing to paths not containing spaces or special characters is recommended. If it doesn't work please try installing the packages to the recommended location before asking for help on the mailing list.</blockquote>

When executing qmake I get errors similar to <code>WARNING: Unable to generate output for: c:/texworks/texworks/Makefile.Debug (TEMPLATE vcapp)</code>. What's wrong?<br>
<blockquote>This is most likely a problem with your Qt version(s) (see also the previous answer). Not all Qt versions have full support for MSVC. Make sure that the one you use (you shouldn't have more than one, anyway) has a C:\KDE\mkspecs\win32-msvc2008 directory. If it does make sure you have all your environmental variables set correctly. If all this doesn't help you can temporarily override the qmake file generation specification by executing<br>
<pre><code>qmake -tp vc -spec C:\KDE\mkspecs\win32-msvc2008<br>
</code></pre></blockquote>

MSVC complains that it can't execute a subprogram or that a subprogram returns an error. Why?<br>
<blockquote>Make sure that you have installed Qt in a path MSVC can find. This includes setting the environmental variables correctly. If you have set the paths in PATH directly (rather than using %KDEDIRS%) make sure your path to Qt doesn't contain spaces or special characters.</blockquote>

MSVC complains about linking errors/unresolved references. Why?<br>
<blockquote>Make sure that you have chosen the Release configuration during compilation (KDE on Windows doesn't provide debugging libraries). If you change the configuration you need choose <b>rebuild</b> for the whole project (just hitting build again isn't enough as the old stuff is still lurking in some files).<br>If you get errors regarding unresolved references make sure you followed this guide completely (setting all paths properly). Also make sure that you followed the <a href='http://techbase.kde.org/Getting_Started/Build/KDE4/Windows/MS_Visual_Studio#Visual_Studio_2008_Express_Edition'>guide from the KDE on Windows project</a>.</blockquote>

When starting TeXworks it complains about missing or incompatible dlls or simply crashes during start-up. What's wrong?<br>
<blockquote>This can be caused if TeXworks doesn't find some required dlls at all or finds different versions of the dlls first. This can most notably be caused by MikTeX which includes some Qt dlls. First of all make sure you have your environmental variables set correctly. If this doesn't help copy the following dlls from C:\KDE\bin to C:\texworks\texworks-read-only\release:<br>
</blockquote><ul><li>freetype.dll<br>
</li><li>jpeg62.dll<br>
</li><li>poppler-qt4.dll<br>
</li><li>QtCore4.dll<br>
</li><li>QtGui4.dll<br>
</li><li>QtXml4.dll<br>
</li><li>zlib1.dll</li></ul>

I have a question not answered here. What shall I do?<br>
<blockquote>Ask the question on the <a href='http://tug.org/mailman/listinfo/texworks'>mailing list</a>.