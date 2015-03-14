_Thanks to Stefan Löffler, Alain Delmotte, Tomek, and Ignasi Furió for developing this guide._



# Preface #

This document describes a procedure for building TeXworks on Windows XP using the (freely available) Minimalist Gnu for Windows (MinGW) suite together with the (also freely available) Minimal System (MSYS) command line system. It is by no means the only way possible and it doesn't anticipate and tackle all possible problems, either. If you run into any difficulties feel free to ask for help on the [mailing list](http://tug.org/mailman/listinfo/texworks) <http://tug.org/mailman/listinfo/texworks>.

_Note: This guide is designed for use with the English version on Windows XP. If you use another language the labels and some of the paths will be different._

# Required Software #

The process described here has been sucessfully implemented using the following software. You may of course use other software, but then the described steps may have to be adapted in some places.
  * Windows XP (+SP3; Vista should work, too, but wasn't tested yet)
  * [MinGW and MSYS](http://www.mingw.org/) <http://www.mingw.org/><br>for building some required libraries and the program itself.<br>Note: This software is for free (as in "speech"). There are some installation notes in the next section.<br>
<ul><li><a href='http://www.7-zip.org/'>7-Zip</a> <<a href='http://www.7-zip.org/>'>http://www.7-zip.org/&gt;</a><br>>= 4.61 for opening tar.gz and tar.lzma<br>Note: This software is for free (as in "beer", mostly also as in "speech")<br>
</li><li><a href='http://subversion.tigris.org/'>Subversion</a> <<a href='http://subversion.tigris.org/>'>http://subversion.tigris.org/&gt;</a><br>for obtaining the TeXworks source code<br>Note: This software is for free (as in "speech")<br>
<i>Note: You need administrator's privileges to install new software.</i></li></ul>

<h1>Setting up MinGW and MSYS</h1>
Start by installing MinGW. For this, go to <a href='http://www.mingw.org/'>http://www.mingw.org/</a> and click on "Downloads" on the left side. On the following page, click on the green "Download Now!" button (as of the time of writing this guide, this downloads "MinGW-5.1.6.exe"). Download the file to your hard disk and execute it. Download and install the following components:<br>
<ul><li>MinGW base tools<br>
</li><li>MinGW make<br>
In this guide, we'll assume you installed them in the folder C:\MinGW.</li></ul>

Then, return to <a href='http://www.mingw.org/'>http://www.mingw.org/</a> and click on "Downloads" again. On the following page, open the "GCC Version 4" folder and the "Current Release" folder inside. From there, download gcc-full-<code>*</code>-mingw32-bin-2.tar.lzma (gcc-full-4.4.0-mingw32-bin-2.tar.lzma at the time of writing). Open the contents with 7-zip and extract the folders to C:\MinGW, replacing existing files.<br>
<br>
To install MSYS, go to <a href='http://www.mingw.org/'>http://www.mingw.org/</a> again and click on "Downloads". This time, scroll down and open the folder "MSYS Base System". In it, open the folder containing the latest version of MSYS (as of the time of writing this guide, this is msys-1.0.11) and click on the file MSYS-<code>*</code>.exe (as of the time of writing this guide, this is MSYS-1.0.11.exe). Download the file to your hard disk and execute it. In this guide, we'll assume you installed MSYS to the folder C:\MSYS\1.0. You'll be asked for the path to MinGW during the install.<br>
<br>
<h1>Directory structure</h1>

The following directory layout is used in this guide. If you choose another layout you have to adjust the paths in the rest of this guide.<br>
<pre><code>C:\MinGW\ (MinGW)<br>
C:\MSYS\1.0 (MSYS)<br>
C:\KDE\ (KDE on Windows libraries)<br>
C:\texworks\hunspell-1.2.8\ (hunspell)<br>
C:\texworks\texworks-read-only\ (TeXworks)<br>
</code></pre>

<h1>Obtaining the necessary libraries from KDE on Windows</h1>

TeXworks depends on several external libraries, most notably Qt and Poppler. You can obtain most of the needed dependencies from <a href='http://windows.kde.org/'>the KDE on Windows project</a> <<a href='http://windows.kde.org/>'>http://windows.kde.org/&gt;</a>. See their <a href='http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps'>summary of steps</a> <<a href='http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps>'>http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps&gt;</a> for further information.<br>
<br>
Download and run the installer. Choose the following options when asked:<br>
<ul><li>Do <b>not</b> skip the basic settings page if asked<br>
</li><li>Installation Directory: Choose a path <b>not containing spaces or special characters</b> (this includes accented characters, umlauts, etc.; only use <a href='http://en.wikipedia.org/wiki/ASCII'>ASCII</a>), e.g. C:\KDE<br>
</li><li>Install Mode: Package Manager<br>
</li><li>Compiler Mode: MinGW4<br>
<i>Note: You need administrator's privileges to install some of the packages.</i></li></ul>

Install the following packages and their dependencies:<br>
<ul><li>freetype (bin),<br>
</li><li>iconv (bin),<br>
</li><li>jpeg (bin),<br>
</li><li>libpng (bin),<br>
</li><li>libxml2 (bin),<br>
</li><li>openjpeg (bin),<br>
</li><li>poppler (bin & devel),<br>
</li><li>poppler-data (bin),<br>
</li><li>qt (bin & devel),<br>
</li><li>zlib (bin & devel)<br>
<i>Note: Not all available servers provide all necessary packages. If the one you chose doesn't, choose another.</i></li></ul>

Add the KDEDIRS environment variable and adjust your path to include the KDE bin and lib directories according to the <a href='http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps'>summary of steps required for installation of KDE on Windows</a> <<a href='http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps>'>http://techbase.kde.org/Projects/KDE_on_Windows/Installation#Summary_of_Steps&gt;</a>. For more information about setting environment variables, see the Q&A below.<br>
<br>
<h1>Downloading and building Hunspell</h1>

The hunspell library is required for spell checking and is not bundled with KDE on Windows. Hence you have to compile it yourself.<br>
<br>
Go to <a href='http://sourceforge.net/projects/hunspell/'>http://sourceforge.net/projects/hunspell/</a> and choose "View all files". On the next page, open the "Hunspell" folder and in it the folder with the latest version number (1.2.8 at the time of writing).<br>
<br>
Download the .tar.gz file (the win32.zip file only contains programs, no sources and no libraries).<br>
<br>
At the time of writing, hunspell-1.2.8.tar.gz was the latest version and will be used throughout this guide.<br>
<br>
Extract hunspell-1.2.8.tar.gz to C:\texworks\<br>
<br>
To compile hunspell, start by running MSYS. In the terminal that appears, type:<br>
<pre><code>cd /c/texworks/hunspell-1.2.8<br>
./configure<br>
cd src/hunspell<br>
make<br>
</code></pre>

There are several warnings but the build should succeed and you can close the MSYS terminal again.<br>
<br>
<h1>Obtaining and building TeXworks</h1>

Run the following command from the command line (i.e. in a DOS prompt window) in the folder C:\texworks<br>
<pre><code>svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only<br>
</code></pre>

You get a fresh new copy of the latest TeXworks code in the folder C:\texworks\texworks-read-only<br>
<br>
In order to build TeXworks you have to modify C:\texworks\texworks-read-only\TeXworks.pro to reflect your local configuration. Find the lines reading<br>
<pre><code>win32 { # paths here are specific to my setup<br>
	INCLUDEPATH += c:/MinGW514/local/include<br>
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
<br>
	RC_FILE = res/TeXworks.rc<br>
<br>
	# for the Windows build, we use static plugins:<br>
	QMAKE_CXXFLAGS += -DSTATIC_SCRIPTING_PLUGINS<br>
}<br>
</code></pre>
and replace them by<br>
<pre><code>win32 {<br>
	INCLUDEPATH += C:/KDE/include/poppler/qt4<br>
	INCLUDEPATH += C:/texworks/hunspell-1.2.8/src/hunspell<br>
	LIBS += -LC:/texworks/hunspell-1.2.8/src/hunspell/.libs<br>
	LIBS += -lpoppler-qt4<br>
	LIBS += -lhunspell-1.2<br>
	LIBS += -lQtUiTools<br>
<br>
	RC_FILE = res/TeXworks.rc<br>
<br>
<br>
	# for the Windows build, we use static plugins:<br>
#	QMAKE_CXXFLAGS += -DSTATIC_SCRIPTING_PLUGINS<br>
}<br>
</code></pre>

In addition, unless you are using a poppler installation that includes the xpdf compatibility headers (not provided by default poppler builds, this requires a configure option), you will need to find the line<br>
<br>
<pre><code>QMAKE_CXXFLAGS += -DHAVE_POPPLER_XPDF_HEADERS<br>
</code></pre>

and comment it out by inserting <code>#</code> at the beginning. The result will be that TeXworks will not automatically locate a poppler-data directory alongside the executable, but will need the poppler data files (for CJK font support) installed in a fixed system location that is dependent on the poppler library build.<br>
<br>
Then run<br>
<pre><code>qmake -makefile -win32 TeXworks.pro<br>
</code></pre>
on the command line from the C:\texworks\texworks-read-only directory. This creates the files Makefile, Makefile.Debug, and Makefile.Release (maybe along with some additional files for internal purposes). To build TeXworks, simply run<br>
<pre><code>mingw32-make<br>
</code></pre>
This should give you a working release\TeXworks.exe.<br>
<br>
If you run into lots of error messages similar to<br>
<pre><code>C:/MinGW/bin/../lib/gcc/mingw32/3.4.5/../../../../include/winbase.h:1681: error:<br>
declaration of C function `LONG InterlockedCompareExchange(volatile LONG*, LONG<br>
, LONG)' conflicts with<br>
../../KDE/include/QtCore/qatomic_windows.h:387: error: previous<br>
declaration `long int InterlockedCompareExchange(long int*, long int, long int)<br>
' here<br>
</code></pre>
refer to the Q&A section below.<br>
<br>
If you run into the error message<br>
<pre><code>c:/mingw/bin/../lib/gcc/mingw32/4.4.0/../../../../mingw32/bin/ld.exe: cannot find -lQtScript<br>
</code></pre>
refer to the Q&A section below.<br>
<br>
<h1>Updating TeXworks</h1>

Updating TeXworks is simple. Just run the following command on the command line from the C:\texworks\texworks-read-only directory:<br>
<pre><code>svn update<br>
qmake<br>
mingw32-make<br>
</code></pre>

If this doesn't help (e.g. you get errors or the application doesn't start), try to prepend the following line:<br>
<pre><code>mingw32-make clean<br>
</code></pre>

<h1>Building plugins</h1>
<h2>Building the Lua scripting plugin</h2>
In order to build the Lua scripting plugin, you need to get the Lua development files suitable for your build setup (mingw + gcc 4.x). The most easiest way to obtain them is from the <a href='http://luaforge.net/frs/?group_id=110'>Lua Binaries</a> <<a href='http://luaforge.net/frs/?group_id=110>'>http://luaforge.net/frs/?group_id=110&gt;</a> page at LuaForge. Download the file <code>lua*_Win32_mingw4_lib.zip</code> (<code>lua5_1_4_Win32_mingw4_lib.zip</code> at the time of writing) and extract it, e.g., to C:\texworks\lua5.1\. Then modify the file plugins-src\TWLuaPlugin\TWLuaPlugin.pro and replace the lines<br>
<pre><code>win32 { # paths here are specific to my setup<br>
	INCLUDEPATH += c:/MinGW514/local/include<br>
<br>
	LIBS += -Lc:/MinGW514/local/lib<br>
	LIBS += -llua5.1<br>
}<br>
</code></pre>
by<br>
<pre><code>win32 {<br>
	INCLUDEPATH += c:/texworks/lua5.1/include<br>
	LIBS += -Lc:/texworks/lua5.1 -llua5.1<br>
	LIBS += -lQtUiTools<br>
}<br>
</code></pre>
Then run <code>qmake</code> and <code>mingw32-make</code> from the command line in the plugins-src\TWLuaPlugin subdirectory. This should leave you with the file plugins-src\TWLuaPlugin\release\libTWLuaPlugin.dll. Now create the directory C:\texworks\texworks-read-only\plugins (if it doesn't exist) and copy the file libTWLuaPlugin.dll into it. Finally restart TeXworks.<br>
<br>
<h2>Building the Python scripting plugin</h2>
In order to build the Python scripting plugin, you need to get a working copy of Python. Downloaded it from <a href='http://www.python.org/'>http://www.python.org/</a> and install it to, e.g., C:\Python26. Then modify the file plugins-src\TWPythonPlugin\TWPythonPlugin.pro and replace the lines<br>
<pre><code>win32 { # paths here are specific to my setup<br>
	INCLUDEPATH += c:/MinGW514/local/include<br>
	INCLUDEPATH += c:/Python26/include<br>
<br>
	LIBS += -Lc:/MinGW514/local/lib<br>
	LIBS += -Lc:/Python26/libs -lpython26<br>
}<br>
</code></pre>
by<br>
<pre><code>win32 {<br>
	INCLUDEPATH += c:/Python26/include<br>
	LIBS += -Lc:/Python26/libs -lpython26<br>
	LIBS += -lQtUiTools<br>
}<br>
</code></pre>
Then run <code>qmake</code> and <code>mingw32-make</code> from the command line in the plugins-src\TWPythonPlugin subdirectory. This should leave you with the file plugins-src\TWPythonPlugin\release\libTWPythonPlugin.dll. Now create the directory C:\texworks\texworks-read-only\plugins (if it doesn't exist) and copy the file libTWPythonPlugin.dll into it. Finally restart TeXworks.<br>
<br>
<h1>Disclaimer</h1>
This guide is provided as-is in the hope that it's helpful. There is no guarantee the described procedure will work on your system. Use at your own risk.<br>
<br>
<h1>Q & A</h1>

How do I add/edit an environment variable?<br>
<blockquote>Right-click on "My Computer" and select "Preferences". Select the "Advanced" tab and click on the button labeled "Environment variables". Note that if an environment variable contains a list of values (e.g. paths), the values are typically separated by a semicolon (";") character.</blockquote>

When running mingw32-make, Windows complains that this command is not recognized. Why?<br>
<blockquote>You need to add C:\MinGW\bin to your PATH environmental variable.</blockquote>

When building TeXworks, I get many errors about conflicting declarations of <code>InterlockedCompareExchange</code>. What's wrong?<br>
<blockquote>This is caused by conflicting declarations of the same function - one by MinGW, the other by the Qt library from KDE on Windows. You need to edit two files to get rid of this error. First, open C:\KDE\include\QtCore\qatomic_windows.h and replace the following lines<br>
<pre><code>   __declspec(dllimport) long __stdcall InterlockedCompareExchange(long *, long, long);<br>
   __declspec(dllimport) long __stdcall InterlockedIncrement(long *);<br>
   __declspec(dllimport) long __stdcall InterlockedDecrement(long *);<br>
   __declspec(dllimport) long __stdcall InterlockedExchange(long *, long);<br>
   __declspec(dllimport) long __stdcall InterlockedExchangeAdd(long *, long);<br>
</code></pre>
by<br>
<pre><code>   __declspec(dllimport) long __stdcall InterlockedCompareExchange(long volatile*, long, long);<br>
   __declspec(dllimport) long __stdcall InterlockedIncrement(long volatile*);<br>
   __declspec(dllimport) long __stdcall InterlockedDecrement(long volatile*);<br>
   __declspec(dllimport) long __stdcall InterlockedExchange(long volatile*, long);<br>
   __declspec(dllimport) long __stdcall InterlockedExchangeAdd(long volatile*, long);<br>
</code></pre></blockquote>

When building TeXworks, I get an error about a missing Qt library of the form ".../ld.exe: cannot find -lQtScript". What's wrong?<br>
<blockquote>This is a bug in the interaction of KDE on Windows, Qt, and MinGW. Qt tells MinGW to look for a library named "QtScript", while KDE on Windows provides a library named "QtScript4". To work around this, create a batch file "fixQtLibs.bat" in C:\texworks\texworks-read-only containing the lines<br>
<pre><code>@echo off<br>
copy "%KDEDIRS%\lib\libQtScript4.a" "%KDEDIRS%\lib\libQtScript.a"<br>
copy "%KDEDIRS%\lib\libQtScriptTools4.a" "%KDEDIRS%\lib\libQtScriptTools.a"<br>
copy "%KDEDIRS%\lib\libQtXml4.a" "%KDEDIRS%\lib\libQtXml.a"<br>
copy "%KDEDIRS%\lib\libQtGui4.a" "%KDEDIRS%\lib\libQtGui.a"<br>
copy "%KDEDIRS%\lib\libQtCore4.a" "%KDEDIRS%\lib\libQtCore.a"<br>
</code></pre>
Then, add the following line to your TeXworks.pro (next to where you edited it before):<br>
<pre><code>QMAKE_PRE_LINK = "fixQtLibs.bat"<br>
</code></pre>
Now the application should build fine. This batch file simply tells the build process to copy the existing files to new files with the correct names. In addition, updates to the KDE on Windows libraries are also used for building TeXworks.<br>
(Note: Instead of using the environmental variable %KDEDIRS% you could also write out the complete path, of course.)</blockquote>

When starting TeXworks, it crashes with some error message about <code>_ZN8QPainter9drawImageERK7QPointFRK6QImage</code>. What's wrong?<br>
<blockquote>This is caused by an incompatibility between the poppler library and the Qt library shipped with KDE on Windows. To get a working copy of the poppler library, go to <a href='http://sourceforge.net/projects/kde-windows/'>http://sourceforge.net/projects/kde-windows/</a> and there click on "View all files". On the following page, open the folder named "poppler", and in it the folder with the latest version (0.12.2 at the time of writing). There, download the file poppler-mingw4-<code>*</code>-bin.tar.bz2 (at the time of writing, this is poppler-mingw4-0.12.2-bin.tar.bz2). From this archive, extract the file bin\libpoppler-qt4.dll to the texworks-read-only\release directory.</blockquote>

Can I mix this compilation using MinGW with the compilation using MSVC?<br>
<blockquote>Yes, although it is strongly advisable to keep the files separate. In order to install the KDE on Windows libraries, you are even required to specify two different paths. For the rest, this is advisable as well. Other than that, there shouldn't be any conflict. Just make sure that the correct programs and libraries are used (check your KDELIBS and/or PATH environmental variable).</blockquote>

QMake doesn't produce any Makefiles or, when subsequently running mingw32-make, it dies with a message about missing separators. Why?<br>
<blockquote>You are probably not using the MinGW version of QMake. Try running "qmake --version". If it doesn't give the path to the MinGW version of the Qt library, you're definitely using the wrong one. Check your KDELIBS and/or PATH environmental variable to include the MinGW version before any other. (Note: this may break the compilation of other programs that rely on the other version of QMake.)</blockquote>

I really want to install the KDE on Windows packages into a folder containing spaces. Can't it be done?<br>
<blockquote>Yes, you can try. If everything works well and all settings are correct it should work on most systems. Some of the workarounds presented here won't work, however. So to be on the safe side, installing to paths not containing spaces or special characters is recommended. If it doesn't work please try installing the packages to the recommended location before asking for help on the mailing list.</blockquote>

When starting TeXworks, it complains about missing or incompatible dlls or simply crashes during start-up. What's wrong?<br>
<blockquote>This can be caused if TeXworks doesn't find some required dlls at all or finds incompatible versions of the dlls, first. This can most notably be caused by MikTeX which includes some Qt dlls. First of all make sure you have your environmental variables set correctly. If this doesn't help, copy the following dlls from C:\KDE\bin to C:\texworks\texworks-read-only\release:<br>
<ul><li>jpeg62.dll<br>
</li><li>libfontconfig.dll<br>
</li><li>libfreetype.dll<br>
</li><li>iconv.dll<br>
</li><li>libopenjpeg.dll<br>
</li><li>libpng12.dll<br>
</li><li>libpoppler-qt4.dll<br>
</li><li>libpoppler.dll<br>
</li><li>libxml2.dll<br>
</li><li>mingwm10.dll<br>
</li><li>QtCore4.dll<br>
</li><li>QtGui4.dll<br>
</li><li>QtScript4.dll<br>
</li><li>QtScriptTools4.dll<br>
</li><li>QtXml4.dll<br>
</li><li>zlib1.dll</li></ul></blockquote>

I have a question not answered here. What shall I do?<br>
<blockquote>Ask the question on the <a href='http://tug.org/mailman/listinfo/texworks'>mailing list</a> <<a href='http://tug.org/mailman/listinfo/texworks>'>http://tug.org/mailman/listinfo/texworks&gt;</a>.