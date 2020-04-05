#!/bin/sh

# ulimit -s 819200

for SVG in **/scalable/*.svg; do
	ICON=$(basename "${SVG}" ".svg")
	CATEGORY=$(dirname $(dirname "${SVG}"))

	case "${CATEGORY}" in
		actions) SIZE=32; SIZE2=64;;
		apps) SIZE=512; SIZE2=0;;
		status) SIZE=48; SIZE2=96;;
	esac

	echo "${CATEGORY}/${ICON}"

	DIR="${CATEGORY}/${SIZE}x${SIZE}"
	mkdir -p "${DIR}"
	inkscape --export-png "${DIR}/${ICON}.png" --export-width=${SIZE} --export-height=${SIZE} --without-gui "${SVG}"

	if [ "${SIZE2}" -gt 0 ]; then
		DIR="${CATEGORY}/${SIZE}x${SIZE}@2"
		mkdir -p "${DIR}"
		inkscape --export-png "${DIR}/${ICON}.png" --export-width=${SIZE2} --export-height=${SIZE2} --without-gui "${SVG}"
	fi
done
