#!/bin/bash

# run at top level of texworks working copy

# ensure we are up-to-date with the repository
svn update

# get the current revision number
REV=`LANG=C svn info | fgrep Revision: | cut -d ' ' -f 2`

if [ `svn status | grep -c "^[^?]"` == 0 ]; then
	# make a new SvnRev.h file
	echo "#define SVN_REVISION $REV" > src/SvnRev.h.new
	echo "#define SVN_REVISION_STR \"$REV\"" >> src/SvnRev.h.new
	# and check if it matches the existing one
	diff src/SvnRev.h.new src/SvnRev.h > /dev/null
	if [ $? == 0 ]; then
		rm src/SvnRev.h.new
		echo "revision is current"
	fi
else
	# if we have local modifications, we'll bump the revision number
	REV=$((1+REV))
	echo "#define SVN_REVISION $REV" > src/SvnRev.h.new
	echo "#define SVN_REVISION_STR \"$REV\"" >> src/SvnRev.h.new
fi

# is there a new revision file?
if [ -e src/SvnRev.h.new ]; then
	mv src/SvnRev.h.new src/SvnRev.h
	VER=`fgrep TEXWORKS_VERSION src/TWVersion.h | cut -d '"' -f 2`
	sed -e "s/@VER@/$VER/;s/@REV@/$REV/;" <TeXworks.plist.in >TeXworks.plist
	cp TeXworks.plist Info.plist
	echo "revision updated"
	svn status # show status, as a reminder to commit the change(s)
fi
