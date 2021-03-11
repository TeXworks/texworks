#!/bin/sh

# ulimit -s 819200

for SVG in **/scalable/*.svg; do
	ICON=$(basename "${SVG}" ".svg")
	CATEGORY=$(dirname $(dirname "${SVG}"))

	SIZE=0
	SIZE2=0
	EXPORTAREA=""
	case "${CATEGORY}" in
		actions) SIZE=32; SIZE2=64;;
		categories) SIZE=32; SIZE2=64;;
	esac

	echo "${CATEGORY}/${ICON}"

	DIR="${CATEGORY}/${SIZE}x${SIZE}"
	mkdir -p "${DIR}"
	if [ ! -f "${DIR}/${ICON}.png" -o "${SVG}" -nt "${DIR}/${ICON}.png" ]; then
		inkscape --export-png "${DIR}/${ICON}.png" --export-width=${SIZE} --export-height=${SIZE} ${EXPORTAREA} --without-gui "${SVG}"
	fi

	if [ "${SIZE2}" -gt 0 ]; then
		DIR="${CATEGORY}/${SIZE}x${SIZE}@2"
		mkdir -p "${DIR}"
		if [ ! -f "${DIR}/${ICON}.png" -o "${SVG}" -nt "${DIR}/${ICON}.png" ]; then
			inkscape --export-png "${DIR}/${ICON}.png" --export-width=${SIZE2} --export-height=${SIZE2} ${EXPORTAREA} --without-gui "${SVG}"
		fi
	fi
done
