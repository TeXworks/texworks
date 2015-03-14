

# Preface #

This document describes a procedure for building TeXworks on Windows using the (freely available) Minimalist Gnu for Windows (MinGW) suite together with the (also freely available) Minimal System (MSYS) command line system. If you just want to use TeXworks (rather than contribute to its development), have a look at the precompiled [Windows builds](https://drive.google.com/folderview?id=0B5iVT8Q7W44pblRNNVVfaXBIcEk&tid=0B5iVT8Q7W44pMkNLblFjUzdQUVE).

The method for compiling TeXworks from source document here is by no means the only one possible and it doesn't anticipate and tackle all possible problems, either. If you run into any difficulties feel free to ask for help on the [mailing list](http://tug.org/mailman/listinfo/texworks) <http://tug.org/mailman/listinfo/texworks>.

Although TeXworks itself has only about 23 MB, the whole setup with programs, tools, and libraries will require roughly 2 GB of disk space.

_Note: This guide was originally designed for the English version on Windows XP. If you use a newer version of Windows or another language, the labels and some of the paths may be different._

## Directory structure ##

The following directory layout is used throughout this guide. If you choose another layout, you have to adjust the paths in the rest of this guide, of course.
```
C:\MinGW\ (MinGW)
C:\MinGW\msys\1.0 (MSYS)
C:\Program Files\GnuWin32 (GNU libraries)
C:\Qt\ (Qt libraries)
C:\texworks\hunspell-1.3.3\ (hunspell sources)
C:\texworks\poppler-0.28.1 (poppler sources)
C:\texworks\texworks-read-only\ (TeXworks)
C:\texworks\libs (libraries)
```
Most of these directories will be created automatically in the procedure outlined below, but `C:\texworks` needs to be created manually.


# Required Software #

The process described here has been sucessfully implemented using the following software.
  * [Subversion](http://subversion.apache.org/) <http://subversion.apache.org/><br>for obtaining the TeXworks source code. Follow one of the links under "Binary Packages" > "Windows" (the command line tools are sufficient)<br>
<ul><li><a href='http://www.cmake.org/'>CMake</a> <<a href='http://www.cmake.org/>'>http://www.cmake.org/&gt;</a><br>is the build system used to build some required libraries and the program itself.<br>
</li><li><a href='http://www.mingw.org/'>MinGW and MSYS</a> <<a href='http://www.mingw.org/>'>http://www.mingw.org/&gt;</a><br>contain the compiler used for building some required libraries and the program itself. There are some installation notes in the next subsection.<br>
</li><li><a href='http://www.7-zip.org/'>7-Zip</a> <<a href='http://www.7-zip.org/>'>http://www.7-zip.org/&gt;</a><br>>= 4.61 for opening tar.gz and tar.xz files.</li></ul>

<i>Note: You need administrator's privileges to install new software.</i>

<h2>Setting up MinGW and MSYS</h2>
Download <a href='http://sourceforge.net/projects/mingw/files/latest/download?source=files'>mingw-get-setup.exe</a> and run it. Follow the instructions to automatically download, install, and run the MinGW Installation Manager. From the MinGW Installation Manager, mark the following packages for installation (click on the box next to the package name and select "Mark for Installation"):<br>
<ul><li>mingw32-base<br>
</li><li>mingw32-gcc-g++<br>
</li><li>msys-base<br>
Install the packages by selecting "Installation" > "Apply Changes" from the menu.</li></ul>

Finally, add <code>C:\MinGW\bin</code> to the PATH environment variable (see Q&A section).<br>
<br>
<br>
<br>
<br>
<h1>Required Libraries</h1>

In addition to the programs mentioned previously, you will also need several libraries that provide functionalities that TeXworks depends on.<br>
<br>
<h2>GNU Libraries</h2>

Several libraries from the GNU project are required to build the other libraries and TeXworks itself. They can be obtained from <a href='http://gnuwin32.sourceforge.net/packages.html'>http://gnuwin32.sourceforge.net/packages.html</a>:<br>
<ul><li>freetype<br>
</li><li>libjpeg<br>
</li><li>libpng<br>
</li><li>libtiff<br>
</li><li>zlib<br>
For each package, click on "Setup" to download the setup file, run it, follow the on-screen instructions, and make sure you install the "Binaries" and "Developer files".</li></ul>

<h2>Hunspell</h2>

Hunspell is used for spell-checking. Its sources can be obtained from <a href='http://hunspell.sourceforge.net/'>http://hunspell.sourceforge.net/</a>. The .tar.gz file can be extracted with 7-Zip to <code>C:\texworks</code>.<br>
<br>
To build hunspell, run MSYS by executing <code>C:\MinGW\msys\1.0\msys.bat</code>. In the terminal window that opens, run the following commands:<br>
<pre><code>cd /c/texworks/hunspell-1.3.3<br>
./configure --prefix=/c/texworks/libs<br>
cd src/hunspell<br>
make install<br>
</code></pre>

This will build hunspell and install it in <code>C:\texworks\libs</code>.<br>
<br>
<h2>Qt</h2>

Qt is the underlying framework that handles all windows, dialogs, and user interaction. To obtain is, go to <a href='http://www.qt.io/download/'>http://www.qt.io/download/</a>, choose the (free) "Community" version (TeXworks is licensed under the GPL), download the Qt Online Installer for Windows, start it, and follow the on-screen instructions. The following components are required:<br>
<ul><li>Qt (any version) > MinGW (32 bit)<br>
</li><li>Tools > Qt Creator<br>
You can safely uncheck all other components to save download bandwith and disk space.</li></ul>

<i>Note: The MinGW version of Qt should match the MinGW version installed above.</i>


<h2>Poppler</h2>

Poppler is used to process and display PDF files. Its sources can be obtained from <a href='http://poppler.freedesktop.org/'>http://poppler.freedesktop.org/</a>. The .tar.xz file can be extracted with 7-Zip to <code>C:\texworks</code>. Also download the poppler encoding data - it will be needed later on - and extract the .tar.gz file to <code>C:\texworks</code>.<br>
<br>
In order to add support for the 14 PDF base fonts (not all of which are available in Windows by default), open the file <code>C:\texworks\poppler-0.28.1\poppler\GlobalParamsWin.cc</code> (e.g., in Qt Creator) and replace the line<br>
<pre><code>        if (dir) {<br>
</code></pre>
by<br>
<pre><code>        if (dir &amp;&amp; displayFontTab[i].t1FileName) {<br>
</code></pre>
and the line<br>
<pre><code>  setupBaseFonts(NULL);<br>
</code></pre>
by<br>
<pre><code>  char fontsPath[MAX_PATH];<br>
  GetModuleFileName(NULL, fontsPath, MAX_PATH);<br>
  unsigned char * p = _mbsrchr ((unsigned char*)fontsPath, '\\');<br>
  *p = '\0';<br>
  strcat(fontsPath, "\\share\\fonts");<br>
  setupBaseFonts(fontsPath);<br>
</code></pre>

To build poppler, run the "CMake (cmake-gui)" from the start menu and set the following values:<br>
<pre><code>Where is the source code: C:/texworks/poppler-0.28.1<br>
Where to build the binaries: C:/texworks/poppler-0.28.1/build<br>
</code></pre>
Then, add the following variables by clicking on "Add Entry":<br>
<pre><code>CMAKE_PREFIX_PATH (string) = C:/Program Files/GnuWin32;C:/Qt/5.3/mingw482_32/lib/cmake<br>
CMAKE_INSTALL_PREFIX (path) = C:/texworks/libs<br>
ENABLE_CPP (bool) = False<br>
ENABLE_XPDF_HEADERS (bool) = True<br>
POPPLER_DATADIR (string) = share/poppler<br>
</code></pre>
If you are working on Windows XP, you additionally need to set<br>
<pre><code>CMAKE_CXX_FLAGS (string) = -D_WIN32_WINNT=0x0500<br>
</code></pre>
Then, click on "Configure". When asked to create the build directory, select "Yes". When asked for a generator, select "MinGW Makefiles" and "Use default native compilers". When the configuration is finished, click "Generate".<br>
<br>
Next, open a command prompt window and execute the following commands:<br>
<pre><code>cd C:\texworks\poppler-0.28.1\build<br>
mingw32-make install<br>
</code></pre>


<h2>Lua (optional)</h2>

Lua is an additional scripting language that can be used in TeXworks, but is optional. If you want to obtain Lua, go to <a href='http://luabinaries.sourceforge.net/download.html'>http://luabinaries.sourceforge.net/download.html</a> and grab the "Windows x86 DLL and Includes (MinGW 4 Comptabible)".<br>
<br>
Unpack the .a file to <code>C:\texworks\libs\lib</code>, the .dll file to <code>C:\texworks\libs\bin</code>, and all files in the include directory to <code>C:\texworks\libs\include</code>.<br>
<br>
<br>
<br>
<br>
<br>
<h1>Building TeXworks</h1>

<h2>Obtaining TeXworks</h2>

In order to get the TeXworks sources, open a command prompt and execute the following commands:<br>
<pre><code>cd C:\texworks<br>
svn checkout http://texworks.googlecode.com/svn/trunk/ texworks-read-only<br>
</code></pre>

<h2>Building TeXworks</h2>

To build TeXworks, start by running "CMake (cmake-gui)" from the start menu and set the following values:<br>
<pre><code>Where is the source code: C:/texworks/texworks-read-only<br>
Where to build the binaries: C:/texworks/texworks-read-only/build<br>
</code></pre>
Then, add the following variables by clicking on "Add Entry":<br>
<pre><code>CMAKE_PREFIX_PATH (string) = C:/Program Files/GnuWin32;C:/Qt/5.3/mingw482_32/lib/cmake;C:/texworks/libs<br>
DESIRED_QT_VERSION (string) = 5<br>
BUILD_SHARED_PLUGINS (bool) = False<br>
</code></pre>
Then, click on "Configure". When asked to create the build directory, select "Yes". When asked for a generator, select "MinGW Makefiles" and "Use default native compilers". The configuration process takes a while, so be patient. When the configuration is finished, click "Generate".<br>
<br>
Next, open a command prompt window and execute the following commands:<br>
<pre><code>cd C:\texworks\texworks-read-only\build<br>
mingw32-make<br>
</code></pre>




Once the compilation finishes, you can find TeXworks.exe in <code>C:\texworks\texworks-read-only\build</code>. To run it, copy the following files to the same directory as TeXworks.exe:<br>
<ul><li><code>C:\Program Files\GnuWin32\bin\freetype6.dll</code>
</li><li><code>C:\Program Files\GnuWin32\bin\jpeg62.dll</code>
</li><li><code>C:\Program Files\GnuWin32\bin\libpng3.dll</code>
</li><li><code>C:\Program Files\GnuWin32\bin\libtiff3.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\icudt52.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\icuin52.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\icuuc52.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\libwinpthread-1.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\Qt5Core.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\Qt5Gui.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\Qt5Script.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\Qt5ScriptTools.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\Qt5Widgets.dll</code>
</li><li><code>C:\Qt\5.3\mingw482_32\bin\Qt5Xml.dll</code>
</li><li><code>C:\texworks\libs\bin\libhunspell-1.3-0.dll</code>
</li><li><code>C:\texworks\libs\bin\libpoppler-qt5.dll</code>
</li><li><code>C:\texworks\libs\bin\libpoppler.dll</code></li></ul>

To be able to correctly display CJK documents, you additionally need to create the folders "share/poppler" in the same directory as TeXworks.exe and copy all folders from <code>C:\texworks\poppler-data-0.4.7</code> into it. To correctly display all 14 PDF base fonts, you need to copy the folder <code>C:\texworks\texworks-read-only\win32\fonts</code> to <code>C:\texworks\texworks-read-only\build\share</code>.<br>
<br>
If you built TeXworks with Lua support, you additionally need to copy<br>
<ul><li><code>C:\texworks\libs\bin\lua52.dll</code>
Note that scripting plugins like Lua are deactivated by default for security reasons. To activate them, go to Edit > Preferences > Scripts and check "Enable plug-in scripting languages".</li></ul>

<h2>Updating TeXworks</h2>

In order to update TeXworks, make sure it is not currently running, open a command prompt and execute the following commands:<br>
<pre><code>cd C:\texworks\texworks-read-only<br>
svn update<br>
mingw32-make<br>
</code></pre>
Usually, this should be sufficient to update the sources and recompile TeXworks. In some rare cases (in particular when there were major changes to the build system), you may need to remove <code>C:\texworks\texworks-read-only\build\CMakeCache.txt</code> and reconfigure the TeXworks build system with cmake-gui as described above.<br>
<br>
<br>
<h1>Q & A</h1>

How do I add/edit an environment variable?<br>
<blockquote>Right-click on "My Computer" and select "Preferences". Select the "Advanced" tab and click on the button labeled "Environment variables". Note that if an environment variable contains a list of values (e.g. paths), the values are typically separated by a semicolon (";") character. If you have any open command prompt windows, you need to close and reopen them before the changes are applied.</blockquote>

When running mingw32-make, Windows complains that this command is not recognized. Why?<br>
<blockquote>You need to add <code>C:\MinGW\bin</code> to your PATH environmental variable.</blockquote>

I have a question not answered here. What shall I do?<br>
<blockquote>Ask the question on the <a href='http://tug.org/mailman/listinfo/texworks'>mailing list</a> <<a href='http://tug.org/mailman/listinfo/texworks>'>http://tug.org/mailman/listinfo/texworks&gt;</a>.