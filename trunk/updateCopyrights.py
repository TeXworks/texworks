#!/usr/bin/python

# This script requires pysvn
# Under Debian/Ubuntu, you can install it via
#    sudo apt-get install python-svn

import pysvn, datetime, re, os, sys

def infoMsg(msg):
	sys.stdout.write(msg)
	sys.stdout.flush()

def svnModified():
	for s in svnClient.status('.', get_all = False):
		if not s['text_status'] in [pysvn.wc_status_kind.none, pysvn.wc_status_kind.unversioned, pysvn.wc_status_kind.normal, pysvn.wc_status_kind.ignored]:
			return True
		if not s['prop_status'] in [pysvn.wc_status_kind.none, pysvn.wc_status_kind.unversioned, pysvn.wc_status_kind.normal, pysvn.wc_status_kind.ignored]:
			return True
	return False

# Uses globals: svnClient, svnLog
def getCopyrightYears(filename):
	global svnClient, svnLog
	
	# Get canonical path (as shown in the changelog)
	info = svnClient.info2(filename)[0][1]
	path = info['URL'].replace(info['repos_root_URL'], "")
	
	minYear = maxYear = None
	
	for rev in svnLog:
		for p in rev['changed_paths']:
			if path == p['path']:
				year = datetime.datetime.utcfromtimestamp(rev['date']).year
				if not minYear or year < minYear:
					minYear = year
				if not maxYear or year > maxYear:
					maxYear = year
				# Follow copies
				if p['copyfrom_path']:
					path = p['copyfrom_path']
				break
	return (minYear, maxYear)
	

# Inspired by http://stackoverflow.com/questions/1597649/replace-strings-in-files-by-python

DEFAULT_REPLACE_EXTENSIONS = (".cpp", ".h")

def try_to_replace(fname, replace_extensions=DEFAULT_REPLACE_EXTENSIONS):
    if replace_extensions:
        return fname.lower().endswith(replace_extensions)
    return True

def replaceInFile(filename):
	infoMsg("Updating %s... " % filename)
	
	# first, see if the pattern is even in the file.
	f = open(filename)
	content = f.read()
	f.close()
	
	m = re.search("(This is part of TeXworks, an environment for working with TeX documents\s*\n\s*Copyright \(C\)) [-0-9]+  ([^\n]+)", content)
	if not m:
		infoMsg("noop\n")
		return

	(yearStart, yearEnd) = getCopyrightYears(filename)
	if not yearStart:
		infoMsg("ERROR\n")
		return
	
	orig = m.group(0)
	if yearStart == yearEnd:
		subst = "%s %i  %s" % (m.group(1), yearStart, m.group(2))
	else:
		subst = "%s %i-%i  %s" % (m.group(1), yearStart, yearEnd, m.group(2))
	
	content = content.replace(orig, subst)
	
	f = open(filename, 'w')
	f.write(content)
	f.close()

	infoMsg("OK\n")




################################################################################
# MAIN
################################################################################

svnClient = pysvn.Client()

# Abort if there are local changes (so if this script should mess things up, it's easy to recover
if svnModified():
	print("Your working copy has local changes. Please commit (or revert) them first")
	sys.exit(1)



# Get the full log
infoMsg("Retrieving svn log... ")
svnLog = svnClient.log('.', discover_changed_paths = True)
infoMsg("OK\n")

# Get all versioned files
infoMsg("Retrieving file list... ")
files = svnClient.list('.', recurse = True)
infoMsg("OK\n")

# The first entry is the directory component
repo_dir = files[0][0]['repos_path']
files = files[1:]

# Update copyright information
for f in files:
	fname = f[0]['repos_path'][len(repo_dir) + 1:]
	if try_to_replace(fname):
		replaceInFile(fname)

# Reminder for places where the copyright information must be updated manually
print("")
print("Don't forget to manually update the copyright information in the following files:")
for f in ["README", "TeXworks.plist.in", "man/texworks.1", "CMake/Modules/COPYING-CMAKE-MODULES", "res/TeXworks.rc", "src/main.cpp", "src/TWApp.cpp"]:
	print("   %s" % f)

