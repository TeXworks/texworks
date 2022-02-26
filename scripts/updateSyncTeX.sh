#!/bin/sh

url4file() {
#	echo "https://github.com/jlaurens/synctex/raw/2020/$1"
	echo "https://www.tug.org/svn/texlive/trunk/Build/source/texk/web2c/synctexdir/$1?view=co"
}

FOLDER="$(dirname "$0")/../modules/synctex"

for FILE in synctex_parser.c synctex_parser.h synctex_parser_utils.c synctex_parser_utils.h synctex_parser_advanced.h synctex_version.h synctex_parser_version.txt; do
	echo -n "Updating $FILE... "
	STAT=$(wget -O "${FOLDER}/${FILE}" "$(url4file ${FILE})" 2>&1)
	if [ $? -eq 0 ]; then
		echo "OK"
	else
		echo "ERROR"
		echo "${STAT}"
		exit 1
	fi
done

