function (GenerateDefaultBinPaths)
	set(DefaultBinPaths "")

	addTeXLiveDefaultBinPaths(DefaultBinPaths)
	addMiKTeXDefaultBinPaths(DefaultBinPaths)
	addSystemDefaultBinPaths(DefaultBinPaths)
	addTeXBinPath(DefaultBinPaths)

	list(REMOVE_DUPLICATES DefaultBinPaths)

	if (NOT WIN32)
		# Windows uses ";" as path separator, just as CMake does for lists
		# *nix systems use ":", so we have to replace the separators
		string(REPLACE ";" ":" _newContent "${DefaultBinPaths}")
	else ()
		set(_newContent "${DefaultBinPaths}")
	endif ()
	set(_newContent "#define DEFAULT_BIN_PATHS \"${_newContent}\"")

	# Generate src/DefaultBinaryPaths.h in the current build directory
	# Only touch the file if it does not exist or if the contents has changed to
	# avoid unnecessary rebuilds of (parts of) the code
	set(_writeFile TRUE)
	if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/src/DefaultBinaryPaths.h")
		file(READ "${CMAKE_CURRENT_BINARY_DIR}/src/DefaultBinaryPaths.h" _oldContent)
		if ("${_oldContent}" STREQUAL "${_newContent}")
			set(_writeFile FALSE)
		endif ()
	endif ()
	if (${_writeFile})
		string(REPLACE ";" "', '" _paths "${DefaultBinPaths}")
		message(STATUS "Generating default binary paths:\n   '${_paths}'")
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/src/DefaultBinaryPaths.h" "${_newContent}")
	endif ()
endfunction (GenerateDefaultBinPaths)

# Binary Directories available in TL (https://www.tug.org/svn/texlive/trunk/Master/bin/)
# aarch64-linux/
# amd64-freebsd/
# amd64-netbsd/
# armhf-linux/
# i386-cygwin/
# i386-freebsd/
# i386-linux/
# i386-netbsd/
# i386-solaris/
# win32/
# x86_64-cygwin/
# x86_64-darwin/
# x86_64-darwinlegacy/
# x86_64-linux/
# x86_64-linuxmusl/
# x86_64-solaris/
function (addTeXLiveDefaultBinPaths pathVar)
	string(TIMESTAMP yearCur "%Y" UTC)
	math(EXPR yearMin "${yearCur} - 5")
	math(EXPR yearMax "${yearCur} + 5")
	if (WIN32)
		set(_path "c:/w32tex/bin")
		foreach(year RANGE ${yearMin} ${yearMax})
			list(INSERT _path 0 "c:/texlive/${year}/bin")
		endforeach()
	else ()
		if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
			set(ARCH "i386")
		else ()
			set(ARCH "x86_64")
		endif ()
		if (CYGWIN)
			set(OS "cygwin")
		elseif (APPLE)
			set(OS "darwin")
		elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
			set(OS "freebsd")
			if ("${ARCH}" STREQUAL "x86_64")
				set(ARCH "amd64")
			endif ()
		elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "NetBSD")
			set(OS "netbsd")
			if ("${ARCH}" STREQUAL "x86_64")
				set(ARCH "amd64")
			endif ()
		elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
			set(OS "solaris")
		# FIXME: darwinlegacy, linuxmusl
		else ()
			set(OS "linux")
		endif ()
		set(_path "")
		foreach(year RANGE ${yearMin} ${yearMax})
			list(INSERT _path 0 "/usr/local/texlive/${year}/bin/${ARCH}-${OS}")
		endforeach()
	endif ()
	list(APPEND ${pathVar} ${_path})
	set(${pathVar} "${${pathVar}}" PARENT_SCOPE)
endfunction (addTeXLiveDefaultBinPaths pathVar)

# MiKTeX
# Windows: Installs to "%LOCALAPPDATA%\Programs\MiKTeX" or "C:\Program Files\MiKTeX"
# (previously, versioned folders such as "C:\Program Files\MiKTeX 2.9" were used)
# Linux: Installs miktex-* binaries to /usr/bin and symlinks them to ~/bin or
# /usr/local/bin (https://miktex.org/howto/install-miktex-unx)
# Mac OS X uses the same symlink locations as Linux (https://miktex.org/howto/install-miktex-mac)
function (addMiKTeXDefaultBinPaths pathVar)
	if (WIN32)
		list(APPEND ${pathVar} "%LOCALAPPDATA%/Programs/MiKTeX/miktex/bin")
		list(APPEND ${pathVar} "%SystemDrive%/Program Files/MiKTeX/miktex/bin")
		list(APPEND ${pathVar} "%SystemDrive%/Program Files (x86)/MiKTeX/miktex/bin")
		foreach(_miktex_version IN ITEMS 3.0 2.9 2.8)
			# TODO: replace hard coded program files path with
			# %ProgramFiles% (might cause problems when running a 32bit application
			# on 64bit Windows) or %ProgramW6432% (added in Win7)
			list(APPEND ${pathVar} "%LOCALAPPDATA%/Programs/MiKTeX ${_miktex_version}/miktex/bin")
			list(APPEND ${pathVar} "%SystemDrive%/Program Files/MiKTeX ${_miktex_version}/miktex/bin")
			list(APPEND ${pathVar} "%SystemDrive%/Program Files (x86)/MiKTeX ${_miktex_version}/miktex/bin")
		endforeach()
	else ()
		list(APPEND ${pathVar} "\${HOME}/bin" "/usr/local/bin")
	endif ()
	set(${pathVar} "${${pathVar}}" PARENT_SCOPE)
endfunction (addMiKTeXDefaultBinPaths pathVar)

function (addTeXBinPath pathVar)
	if (CMAKE_CROSSCOMPILING)
		return()
	endif ()
	if (WIN32)
		get_filename_component(_tex tex.exe PROGRAM)
	else ()
		get_filename_component(_tex tex PROGRAM)
	endif ()
	if (NOT _tex)
		return()
	endif ()
	get_filename_component(_path "${_tex}" DIRECTORY)
	list(INSERT ${pathVar} 0 "${_path}")
	set(${pathVar} "${${pathVar}}" PARENT_SCOPE)
endfunction (addTeXBinPath pathVar)

function (addSystemDefaultBinPaths pathVar)
	if (APPLE)
		list(INSERT ${pathVar} 0 "/Library/TeX/texbin" "/usr/texbin")
	endif ()
	if (UNIX)
		list(APPEND ${pathVar} "/usr/local/bin" "/usr/bin")
	endif ()
	set(${pathVar} "${${pathVar}}" PARENT_SCOPE)
endfunction (addSystemDefaultBinPaths pathVar)
