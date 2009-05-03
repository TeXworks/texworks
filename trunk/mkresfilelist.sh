#!/bin/bash

# this script is currently only set up to run on the Mac....
QTDIR=/Developer/Applications/Qt

# the resource file list that we will build
RESFILES=res/resfiles.qrc

# first, ensure that all translations specified in the .pro are compiled
lrelease -compress TeXworks.pro

# collect any corresponding Qt translations for the languages we know about
for f in trans/TeXworks_*.qm; do
	f=${f#trans/}
	q=${QTDIR}/${f/TeXworks/qt}
	if [ -e ${q} ]; then
		cp ${q} res/resfiles/translations/
    else
        # no Qt-supplied file, check for a local qt_* file as well
        q=trans/${f/TeXworks/qt}
        if [ -e ${q} ]; then
			cp ${q} res/resfiles/translations/
		fi
	fi
done

# other resource files are currently maintained by hand directly in the resfiles directories

# build the resfiles list
echo '<RCC>' > $RESFILES
echo '<qresource>' >> $RESFILES
# list all the hand-maintained resource files in the source tree
find res/resfiles -type f -print | fgrep -v .svn | fgrep -v .DS_Store | sed -e 's!res/\(.*\)!<file>\1</file>!' >> $RESFILES
# list the compiled translations from the trans directory
find trans -name 'TeXworks_*.qm' | sed -e 's!trans/\(.*\)!<file alias="resfiles/translations/\1">../trans/\1</file>!' >> $RESFILES
echo '</qresource>' >> $RESFILES
echo '</RCC>' >> $RESFILES
