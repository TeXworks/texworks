#!/usr/bin/env sh

# Exit on errors
set -e

UNSUPPORTED_SERIES="precise"

print_error() {
	echo "::error file=${0}::${1}"
}
print_warning() {
	echo "::warning file=${0}::${1}"
}
echo_var () {
	# echo "$1 = ${!1}" does not work in dash (Ubuntu's default sh)
	eval "echo \"$1 = \$$1\""
}

# Gather information

echo "::group::Info"

# GNU extensions for sed are not supported; on Linux, --posix mimicks this behaviour
TW_VERSION=$(sed -ne 's,^#define TEXWORKS_VERSION[[:space:]]"\([0-9.]\{3\,\}\)"$,\1,p' src/TWVersion.h)
echo "TW_VERSION = ${TW_VERSION}"

GIT_HASH=$(git --git-dir=".git" show --no-patch --pretty="%h")
echo "GIT_HASH = ${GIT_HASH}"

DATE_HASH=$(date -u +"%Y%m%d%H%M")
echo "DATE_HASH = ${DATE_HASH}"

DEBDATE=$(date -R)

echo_var "DEBDATE"
echo_var "DEB_MAINTAINER_NAME"
echo_var "DEB_MAINTAINER_EMAIL"
echo_var "LAUNCHPAD_SERIES"

echo "::endgroup::"

if [ -z "${DEB_MAINTAINER_NAME}" -o -z "${DEB_MAINTAINER_EMAIL}" -o -z "${DEB_PASSPHRASE}" -o -z "${LAUNCHPAD_SERIES}" ]; then
	print_error "DEB_MAINTAINER_NAME and/or DEB_MAINTAINER_EMAIL and/or DEB_PASSPHRASE and/or LAUNCHPAD_SERIES are not set"
	exit 1
fi

BUILDDIR="launchpad-build"
ORIG_VERSION="${TW_VERSION}~${DATE_HASH}~git~${GIT_HASH}"
ORIGNAME="texworks_${ORIG_VERSION}"
ORIGFILENAME="${ORIGNAME}.orig.tar.gz"

echo "   exporting sources to ${BUILDDIR}/${ORIGFILENAME}"
mkdir "${BUILDDIR}"
git archive --prefix="${ORIGNAME}/" --output="${BUILDDIR}/${ORIGFILENAME}" HEAD

TOPDIR=$(pwd)

OUTPUT_FILES=""

for SERIES in ${LAUNCHPAD_SERIES}; do
	SUPPORTED=1
	for US in ${UNSUPPORTED_SERIES}; do
		if [ "${SERIES}" = "${US}" ]; then SUPPORTED=0; fi
	done
	if [ ${SUPPORTED} -eq 0 ]; then
		echo "Skipping ${SERIES} - unsupported"
		continue
	fi

	echo "::group::Packaging for ${SERIES}"
	DEB_VERSION="${ORIG_VERSION}-1${SERIES}"
	echo_var "DEB_VERSION"

	DEBDIR="${BUILDDIR}/texworks-${DEB_VERSION}"
	echo "exporting sources to ${DEBDIR}"
	mkdir -p "${DEBDIR}"
	tar -x -C "${DEBDIR}" --strip-components=1 -f "${BUILDDIR}/${ORIGFILENAME}"

	echo "copying debian directory"
	cp -r ".github/actions/package-launchpad/launchpad/debian" "${DEBDIR}"

	PATCHFILE=".github/actions/package-launchpad/launchpad/${SERIES}.patch"
	if [ -f "${PATCHFILE}" ]; then
		echo "   applying ${SERIES}.patch"
		patch -d "${DEBDIR}" -p0 < "${PATCHFILE}"
	fi


	echo "preparing copyright"
	sed -i -e "s/<AUTHOR>/${DEB_MAINTAINER_NAME}/g" -e "s/<DATE>/${DEBDATE}/g" "${DEBDIR}/debian/copyright"

	echo "preparing changelog"
	printf "texworks (${DEB_VERSION}) ${SERIES}; urgency=low\n\n" > "${DEBDIR}/debian/changelog"
	case "${GITHUB_REF}" in
		refs/tags/*)
			NEWS=$(sed -n "/^Release ${TW_VERSION}/,/^Release/p" "NEWS" | sed -e '/^Release/d' -e 's/^\t/    /')
			echo "$NEWS" >> "${DEBDIR}/debian/changelog"
			;;
		*)
			git log --reverse --pretty=format:"%w(80,4,6)* %s" "${PREV_COMMIT}.." >> "${DEBDIR}/debian/changelog"
			echo "" >> "${DEBDIR}/debian/changelog" # git log does not append a newline
			;;
	esac
	printf "\n -- ${DEB_MAINTAINER_NAME} <${DEB_MAINTAINER_EMAIL}>  ${DEBDATE}\n" >> "${DEBDIR}/debian/changelog"

	echo "building package"
	echo ""
	cd "${DEBDIR}"

	PASSPHRASE_FILE="${TOPDIR}/passphrase.txt"
	touch "${PASSPHRASE_FILE}"
	chmod 600 "${PASSPHRASE_FILE}"
	echo "${DEB_PASSPHRASE}" > "${TOPDIR}/passphrase.txt" || print_error "Failed to create passphrase.txt"
	debuild -k8740ED04AF6A4FCC6BC51C426806F10000582F84 -p"gpg --no-tty --batch --pinentry-mode=loopback  --passphrase-file ${PASSPHRASE_FILE}" -d -S && DEBUILD_RETVAL=$? || DEBUILD_RETVAL=$?
	rm -f "${PASSPHRASE_FILE}"

	if [ $DEBUILD_RETVAL -ne 0 ]; then
		print_warning "   debuild failed with status code ${DEBUILD_RETVAL}"
		continue
	fi
	cd "${TOPDIR}"

	OUTPUT_FILES="${BUILDDIR}/texworks_${DEB_VERSION}_source.changes ${OUTPUT_FILES}"
	echo "::endgroup::"
done

echo "::set-output name=CHANGES::${OUTPUT_FILES}"

echo "Packaging completed"
