RESFILES=resfiles.qrc
echo '<RCC>' > $RESFILES
echo '<qresource>' >> $RESFILES
find resfiles -type f -print | fgrep -v .svn | fgrep -v .DS_Store | sed -e 's!\(.*\)!<file>\1</file>!' >> $RESFILES
echo '</qresource>' >> $RESFILES
echo '</RCC>' >> $RESFILES
