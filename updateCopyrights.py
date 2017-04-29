#!/usr/bin/env python2
#
# System requirements:
#  - GitPython: https://pypi.python.org/pypi/GitPython/
#
from __future__ import print_function
from git import Repo
import datetime, os, re

# Global variable for the git repository
gitrepo = None

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

    RE_PART_OF_TEXWORKS = "(This is part of TeXworks, an environment for working with TeX documents\s*\n\s*Copyright \(C\)) [-0-9]+  ([^\n]+)"
    REPLACE_EXTENSIONS = ['.cpp', '.h']

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

    @property
    def year_range_str(self):
        """Returns the year (range) in which this file was modified"""
        global gitrepo
        timestamp_str = gitrepo.log('--diff-filter=A', '--follow', '--format="%at"', '--', self.filename)
        timestamp = float(timestamp_str.replace('"', ''))
        begin = datetime.datetime.fromtimestamp(timestamp).year
        logs = gitrepo.log('--format="%at %s"', '--', self.filename).split('\n')
        for log in logs:
        	log = log[1:-1]
        	log = log.split(' ', 1)
        	if log[1] in ['Update copyrights', 'Update copyright notices', 'Update copyright statements', 'Updated copyright information', 'Update copyright and add Charlie to list of authors', 'Update URLs to http://www.tug.org/texworks/', 'update copyright to 2010']: continue
        	timestamp_str = log[0]
        	print('   %s: %s' % (self.filename, log[1]))
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

    def needs_update(self):
        """Returns True if the file needs updating, False otherwise."""
        if not self.one_of_replace_extensions():
            return False

        self.content = open(self.filename).read()
        self.matches = re.search(self.RE_PART_OF_TEXWORKS, self.content)
        return self.matches

    def update_copyright(self):
        """Updates the copyright line in the file."""

        # Replace the year(s)
        orig = self.matches.group(0)
        subst = "{0} {1}  {2}".format(self.matches.group(1), self.year_range_str, self.matches.group(2))
        self.content = self.content.replace(orig, subst)

        # Write the contents to disk
        f = open(self.filename, 'w')
        f.write(self.content)
        f.close()

        print("Updated {0}".format(self.filename))

def manual_update_notice():
    """Reminder for places where the copyright information must be updated manually"""
    print("")
    print("Don't forget to manually update the copyright information in the following files:")
    for f in ["README.md", "TeXworks.plist.in", "man/texworks.1", "CMake/Modules/COPYING-CMAKE-MODULES", "res/TeXworks.rc", "src/main.cpp", "src/TWApp.cpp", "travis-ci/launchpad/debian/copyright", "travis-ci/README.win"]:
    	print("   {0}".format(f))

def main():
    """Main"""
    global gitrepo
    gitrepo = Repo(".").git
    for root, dirs, files in os.walk('.'):
        for f in files:
            the_file = CopyrightedFile(os.path.join(root, f))
            if the_file.needs_update():
                the_file.update_copyright()

    manual_update_notice()

if __name__ == '__main__':
    main()
