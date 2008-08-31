#!/bin/bash

# this script is run on the Mac....
QTDIR=/Developer/Applications/Qt

RESFILES=resfiles.qrc

# collect any compiled translations from the trans source folder
mkdir -p resfiles/translations
find ../trans -name "*.qm" -exec cp {} resfiles/translations/ \;

# collect any corresponding Qt translations we have available
for f in resfiles/translations/TeXworks_*.qm; do
	f=${f#resfiles/}
	q=${QTDIR}/${f/TeXworks/qt}
	if [ -e ${q} ]; then
		cp ${q} resfiles/translations/
	fi
done

# other resource files are currently maintained by hand directly in the resfiles directories

# build the resfiles list
echo '<RCC>' > $RESFILES
echo '<qresource>' >> $RESFILES
find resfiles -type f -print | fgrep -v .svn | fgrep -v .DS_Store | sed -e 's!\(.*\)!<file>\1</file>!' >> $RESFILES
echo '</qresource>' >> $RESFILES
echo '</RCC>' >> $RESFILES
