#!/usr/bin/env python3
#
# System requirements:
#  - GitPython: https://pypi.python.org/pypi/GitPython/
#
from __future__ import print_function
import git
from git import Repo
import datetime, os, re

# Global variable for the git repository
gitrepo = None

class FileNotVersionedError(Exception):
	def __init__(self, filename):
		super(FileNotVersionedError, self).__init__("File '%s' is not versioned" % filename)

class CopyrightedFile:
    """A thin wrapper for a real file on the filesystem. This class assists
    in updating the file's embedded copyright information.

    Attributes
    ----------

    RE_PART_OF_TEXWORKS: string
                         A regular expression to match the embedded copyright
                         line in files.
    REPLACE_EXTENSIONS: list[string]
                        Files ending in any of these extensions will be
                        considered when updating copyrighted files.

    """

    RE_PART_OF_TEXWORKS = "(\s*Copyright \(C\)) [-0-9]+  ([^\n]+)"
    REPLACE_EXTENSIONS = ['.cpp', '.h']
    EXCLUDE_FOLDERS = ['./modules/synctex/']

    def __init__(self, filename):
        """Construct from filename.

        Parameters
        ----------
        filename: str
                  Path to a file. This file's copyright information will be
                  examined and possibly updated by using the helper methods
                  below.

        """
        self.filename = filename
        self.log = None

    @property
    def year_range_str(self):
        """Returns the year (range) in which this file was modified"""
        global gitrepo
        timestamp_str = gitrepo.log('--diff-filter=A', '--follow', r'--format="%at"', '--', self.filename)
        if len(timestamp_str.strip()) == 0: raise FileNotVersionedError(self.filename)
        timestamp = float(timestamp_str.replace('"', ''))
        begin = datetime.datetime.fromtimestamp(timestamp).year
        logs = gitrepo.log('--format="%h %at %s"', '--', self.filename).split('\n')
        for log in logs:
            log = log[1:-1]
            commit, timestamp_str, msg = log.split(' ', 2)
            if msg in ['Update copyright', 'Update copyrights', 'Update copyright notices', 'Update copyright statements', 'Updated copyright information', 'Update copyright and add Charlie to list of authors', 'Update URLs to http://www.tug.org/texworks/', 'update copyright to 2010']: continue
            self.log = (commit, timestamp_str, msg)
            break
#        print(logs)
#        timestamp_str = gitrepo.log('-1', '--format="%at"', '--', self.filename)
        timestamp = float(timestamp_str.replace('"', ''))
        end = datetime.datetime.fromtimestamp(timestamp).year
        if begin == end:
            return str(end)
        else:
            return "{0}-{1}".format(begin, end)


    def one_of_replace_extensions(self):
        """Returns True if the file extension is allowed to be updated, False otherwise."""
        for ext in self.REPLACE_EXTENSIONS:
            if self.filename.endswith(ext):
                return True

        return False

    def is_excluded(self):
        for d in self.EXCLUDE_FOLDERS:
            if self.filename.startswith(d):
                return True
        return False

    def is_updateable(self):
        return not self.is_excluded() and self.one_of_replace_extensions()

    def needs_update(self):
        """Returns True if the file needs updating, False otherwise."""

        self.content = open(self.filename).read()
        self.matches = re.search(self.RE_PART_OF_TEXWORKS, self.content[:1000])
        if self.matches is None:
            print('    [WARNING] No Copyright notice found in {0}'.format(self.filename))
        return self.matches

    def update_copyright(self):
        """Updates the copyright line in the file."""

        # Replace the year(s)
        orig = self.matches.group(0)
        try:
	        subst = "{0} {1}  {2}".format(self.matches.group(1), self.year_range_str, self.matches.group(2))
        except FileNotVersionedError:
            return

        if subst == orig:
            print('   Already up to date')
            return
        self.content = self.content.replace(orig, subst)

        # Write the contents to disk
        f = open(self.filename, 'w')
        f.write(self.content)
        f.close()

        timestamp = datetime.datetime.fromtimestamp(float(self.log[1].replace('"', ''))).strftime('%c')

        print("   Last commit: {0} ({1}) {2}".format(self.log[0], timestamp, self.log[2]))
        print("   Updated {0}".format(self.filename))

def manual_update_notice():
    """Reminder for places where the copyright information must be updated manually"""
    print("")
    print("Don't forget to manually update the copyright information in the following files:")
    for f in ["README.md", "res/texworks.appdata.xml", "res/TeXworks.plist.in", "res/texworks.1", "res/TeXworks.rc", "src/TWApp.cpp", ".github/actions/package-launchpad/launchpad/debian/copyright", "win32/README.win", "CMake/Modules/COPYING-CMAKE-MODULES"]:
    	print("   {0}".format(f))

def main():
    """Main"""
    GIT_ROOT = os.path.join(os.path.dirname(__file__), '..')

    global gitrepo
    gitrepo = Repo(GIT_ROOT).git

    for f in git.cmd.Git().ls_files(GIT_ROOT).split():
        the_file = CopyrightedFile(f)
        if not the_file.is_updateable(): continue
        print(f)
        if the_file.needs_update():
            the_file.update_copyright()
        else:
            print('    Skipping')

    manual_update_notice()

if __name__ == '__main__':
    main()
