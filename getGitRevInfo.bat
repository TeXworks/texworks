@echo off

rem Use git to create src/GitRev.h.new with latest commit information. If this
rem fails (e.g., because git is not available), we assume that this is an export
rem so the commit info is in GitArchiveInfo.txt
git --git-dir=".git" show --no-patch --pretty="// This file is used to identify the latest git commit. Please do not touch.%%n#define GIT_COMMIT_HASH \"%%h\"%%n#define GIT_COMMIT_DATE \"%%ci\"%%n" > src\GitRev.h.new 2> nul || copy GitArchiveInfo.txt src\GitRev.h.new > nul

rem If src/GitRev.h does not exist, yet, we simply create it now
if not exist src\GitRev.h (
	rename src\GitRev.h.new GitRev.h
	exit /B 0
)

rem If we get here, git ran successfully and we have src/GitRev.h.new with the new
rem version information
rem If it's the same as src/GitRev.h, there's nothing left to do except remove the
rem temporary src/GitRev.h.new and finish
fc src\GitRev.h src\GitRev.h.new > nul
if %ERRORLEVEL% == 0 (
	del src\GitRev.h.new 2> nul
	exit /B 0
)

rem If we get here, src/GitRev.h and src/GitRev.h.new are not the same (or a
rem problem occured) so we take the new file
rename src\GitRev.h.new GitRev.h

