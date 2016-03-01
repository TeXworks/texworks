#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

print_headline "Packaging ${TARGET_OS}/qt${QT} for deployment"

POPPLERDATA_VERSION="0.4.7"
POPPLERDATA_SUBDIR="poppler-data-${POPPLERDATA_VERSION}"
POPPLERDATA_FILE="poppler-data-${POPPLERDATA_VERSION}.tar.gz"
POPPLERDATA_URL="https://poppler.freedesktop.org/${POPPLERDATA_FILE}"
POPPLERDATA_SHA256="e752b0d88a7aba54574152143e7bf76436a7ef51977c55d6bd9a48dccde3a7de"

# Gather information

# GNU extensions for sed are not supported; on Linux, --posix mimicks this behaviour
TW_VERSION=$(sed -ne 's,^#define TEXWORKS_VERSION[[:space:]]"\([0-9.]\{3\,\}\)"$,\1,p' src/TWVersion.h)
echo "TW_VERSION = ${TW_VERSION}"

GIT_HASH=$(git --git-dir=".git" show --no-patch --pretty="%h")
echo "GIT_HASH = ${GIT_HASH}"

GIT_DATE=$(git --git-dir=".git" show --no-patch --pretty="%ci")
echo "GIT_DATE = ${GIT_DATE}"

DATE_HASH=$(date -u +"%Y%m%d%H%M")
echo "DATE_HASH = ${DATE_HASH}"

if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
	RELEASE_DATE=$(date -u +"%Y-%m-%dT%H:%M:%S%z" --date="${GIT_DATE}")
elif [ "${TRAVIS_OS_NAME}" = "osx" ]; then
	RELEASE_DATE=$(date -ujf "%Y-%m-%d %H:%M:%S %z" "${GIT_DATE}" "+%Y-%m-%dT%H:%M:%S%z")
else
	print_error "Unsupported operating system '${TRAVIS_OS_NAME}'"
	exit 1
fi
echo "RELEASE_DATE = ${RELEASE_DATE}"

#VERSION_NAME="TeXworks-${TRAVIS_OS_NAME}-${TW_VERSION}-${DATE_HASH}-git_${GIT_HASH}"
VERSION_NAME="${TW_VERSION}-t${DATE_HASH}-git_${GIT_HASH}"
echo "VERSION_NAME = ${VERSION_NAME}"

# Start packaging and prepare deployment
cd "${BUILDDIR}"

if [ "${TARGET_OS}" = "linux" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	echo "Not packaging for ${TARGET_OS}/qt${QT}"
elif [ "${TARGET_OS}" = "win" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	if [ ${QT} -eq 4 ]; then
		print_info "Not packaging for ${TARGET_OS}/qt${QT}"
	elif [ ${QT} -eq 5 ]; then
		print_info "Stripping TeXworks.exe"
		${MXEDIR}/usr/bin/${MXETARGET}-strip ${BUILDDIR}/TeXworks.exe
		print_info "Assembling package"
		echo_and_run "mkdir -p \"package-zip/share\""
		echo_and_run "cp \"${BUILDDIR}/TeXworks.exe\" \"package-zip/\""
		echo_and_run "cp \"${TRAVIS_BUILD_DIR}/COPYING\" \"package-zip/\""
		echo_and_run "cp -r \"${TRAVIS_BUILD_DIR}/win32/fonts\" \"package-zip/share/\""
		# FIXME: manual (only for tags)
		echo_and_run "cp -r \"${TRAVIS_BUILD_DIR}/travis-ci/README.win\" \"package-zip/README.txt\""

		print_info "Fetching poppler data"
		wget "${POPPLERDATA_URL}"
		CHKSUM=$(openssl dgst -sha256 "${POPPLERDATA_FILE}" 2> /dev/null)
		if [ "${CHKSUM}" != "SHA256(${POPPLERDATA_FILE})= ${POPPLERDATA_SHA256}" ]; then
			print_error "Wrong checksum"
			print_error "${CHKSUM}"
			print_error "(expected: ${POPPLERDATA_SHA256})"
			exit 1
		fi
		echo_and_run "tar -x -C \"package-zip/share/\" -f \"${BUILDDIR}/${POPPLERDATA_FILE}\" && mv \"package-zip/share/${POPPLERDATA_SUBDIR}\" \"package-zip/share/poppler\""

		print_info "zipping '${TRAVIS_BUILD_DIR}/TeXworks-${TARGET_OS}-${VERSION_NAME}.zip'"
		echo_and_run "cd package-zip && zip -r \"${BUILDDIR}/TeXworks-${TARGET_OS}-${VERSION_NAME}.zip\" *"

		# FIXME: installer (only for tags)

		print_info "Preparing bintray.json"

		cat > "${TRAVIS_BUILD_DIR}/travis-ci/bintray.json" <<EOF
		{
			"package": {
				"name": "TeXworks",
				"repo": "Windows-latest",
				"subject": "texworks"
			},
			"version": {
				"name": "${VERSION_NAME}",
				"released": "${RELEASE_DATE}",
				"gpgSign": true
			},
			"files":
			[
				{"includePattern": "${BUILDDIR}/TeXworks-${TARGET_OS}-${VERSION_NAME}.zip", "uploadPattern": "TeXworks-${TARGET_OS}-${VERSION_NAME}.zip"}
			],
			"publish": true
		}
EOF
	else
		print_error "Skipping unsupported combination '${TARGET_OS}/qt${QT}'"
	fi
elif [ "${TARGET_OS}" = "osx" -a "${TRAVIS_OS_NAME}" = "osx" ]; then
	if [ ${QT} -eq 4 ]; then
		print_info "Running CPack"
		cpack --verbose

		print_info "Preparing bintray.json"
		cat > "${TRAVIS_BUILD_DIR}/travis-ci/bintray.json" <<EOF
		{
			"package": {
				"name": "TeXworks",
				"repo": "OSX-latest",
				"subject": "texworks"
			},
			"version": {
				"name": "${VERSION_NAME}",
				"released": "${RELEASE_DATE}",
				"gpgSign": true
			},
			"files":
			[
				{"includePattern": "${BUILDDIR}/(TeXworks.*\\\\.dmg)", "uploadPattern": "TeXworks-${TRAVIS_OS_NAME}-${VERSION_NAME}.dmg"}
			],
			"publish": true
		}
EOF
	elif [ ${QT} -eq 5 ]; then
		print_info "Not packaging for ${TARGET_OS}/qt${QT}"
	else
		print_error "Skipping unsupported combination '${TARGET_OS}/qt${QT}'"
	fi
else
	print_error "Skipping unsupported host/target combination '${TRAVIS_OS_NAME}/${TARGET_OS}'"
fi

cd "${TRAVIS_BUILD_DIR}"

print_info "Deployment preparation successful"
