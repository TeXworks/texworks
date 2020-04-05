#!/bin/sh

# ulimit -s 819200

for SVG in **/scalable/*.svg; do
	ICON=$(basename "${SVG}" ".svg")
	CATEGORY=$(dirname $(dirname "${SVG}"))

	case "${CATEGORY}" in
		actions) SIZE=32;;
		apps) SIZE=512;;
		status) SIZE=48;;
	esac

	echo "${CATEGORY}/${ICON}"

	DIR="${CATEGORY}/${SIZE}x${SIZE}"
	mkdir -p "${DIR}"
	inkscape --export-png "${DIR}/${ICON}.png" --export-width=${SIZE} --export-height=${SIZE} --without-gui "${SVG}"
done
