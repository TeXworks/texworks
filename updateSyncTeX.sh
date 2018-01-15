#!/bin/sh

for FILE in synctex_parser.c synctex_parser.h synctex_parser_utils.c synctex_parser_utils.h synctex_parser_advanced.h synctex_version.h synctex_parser_version.txt; do
	echo -n "Updating $FILE... "
	STAT=$(wget -O "src/${FILE}" "https://github.com/jlaurens/synctex/raw/2017/${FILE}" 2>&1)
	if [ $? -eq 0 ]; then
		echo "OK"
	else
		echo "ERROR"
		echo "$STAT"
		exit 1
	fi
done

