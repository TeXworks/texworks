#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
	print_warning "Not packaging pull-requests for deployment"
	exit 0
fi

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
VERSION_NAME="${TW_VERSION}-${DATE_HASH}-git_${GIT_HASH}"
echo "VERSION_NAME = ${VERSION_NAME}"

# Start packaging and prepare deployment
cd "${BUILDDIR}"

if [ "${TARGET_OS}" = "linux" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	if [ ${QT} -eq 4 ]; then
		DEBDATE=$(date -R)

		echo_var "DEBDATE"
		echo_var "DEB_MAINTAINER_NAME"
		echo_var "DEB_MAINTAINER_EMAIL"
		if [ -z "${DEB_MAINTAINER_NAME}" -o -z "${DEB_MAINTAINER_EMAIL}" -o -z "${DEB_PASSPHRASE}" -o -z "${LAUNCHPAD_DISTROS}" ]; then
			print_error "DEB_MAINTAINER_NAME and/or DEB_MAINTAINER_EMAIL and/or DEB_PASSPHRASE and/or LAUNCHPAD_DISTROS are not set"
			exit 0
		fi
		openssl aes-256-cbc -K $encrypted_54846cac3f0f_key -iv $encrypted_54846cac3f0f_iv -in "${TRAVIS_BUILD_DIR}/travis-ci/launchpad/key.asc.enc" -out "${TRAVIS_BUILD_DIR}/travis-ci/launchpad/key.asc" -d
		gpg --import "${TRAVIS_BUILD_DIR}/travis-ci/launchpad/key.asc"

		for DISTRO in ${LAUNCHPAD_DISTROS}; do
			print_info "Packging for ${DISTRO}"
			DEB_VERSION=$(echo "${VERSION_NAME}" | tr "_-" "~")"~${DISTRO}"
			echo -n "   "
			echo_var "DEB_VERSION"

			DEBDIR="${BUILDDIR}/texworks-${DEB_VERSION}"
			print_info "   exporting sources to ${DEBDIR}"
			mkdir -p "${DEBDIR}"
			cd "${TRAVIS_BUILD_DIR}" && git archive --format=tar HEAD  | tar -x -C "${DEBDIR}" -f -

			print_info "   copying debian directory"
			cp -r "${TRAVIS_BUILD_DIR}/travis-ci/launchpad/debian" "${DEBDIR}"

			if [ -f "${TRAVIS_BUILD_DIR}/travis-ci/launchpad/${DISTRO}.patch" ]; then
				print_info "   applying ${DISTRO}.patch"
				patch -d "${DEBDIR}" -p0 < "${TRAVIS_BUILD_DIR}/travis-ci/launchpad/${DISTRO}.patch"
			fi


			print_info "   preparing copyright"
			sed -i -e "s/<AUTHOR>/${DEB_MAINTAINER_NAME}/g" -e "s/<DATE>/${DEBDATE}/g" "${DEBDIR}/debian/copyright"

			print_info "   preparing changelog"
			echo "texworks (${DEB_VERSION}) ${DISTRO}; urgency=low\n" > "${DEBDIR}/debian/changelog"
			if [ -z "${TRAVIS_TAG}" ]; then
				git log --reverse --pretty=format:"%w(80,4,6)* %s" ${TRAVIS_COMMIT_RANGE} >> "${DEBDIR}/debian/changelog"
				echo "" >> "${DEBDIR}/debian/changelog" # git log does not append a newline
			else
				NEWS=$(sed -n "/^Release ${TW_VERSION}/,/^Release/p" ${TRAVIS_BUILD_DIR}/NEWS | sed -e '/^Release/d' -e 's/^\t/    /')
				echo "$NEWS" >> "${DEBDIR}/debian/changelog"
			fi
			echo "\n -- ${DEB_MAINTAINER_NAME} <${DEB_MAINTAINER_EMAIL}>  ${DEBDATE}" >> "${DEBDIR}/debian/changelog"

			print_info "   building package"
			cd "${DEBDIR}"

			echo -n "" > "/tmp/passphrase.txt" || print_error "Failed to create /tmp/passphrase.txt"
			# Write the passphrase to the file several times; debuild (debsign)
			# will try to sign (at least) the .dsc file and the .changes files,
			# thus reading the passphrase from the pipe several times
			# NB: --passphrase-file seems to be broken somehow
			for i in $(seq 10); do
				echo "${DEB_PASSPHRASE}" >> "/tmp/passphrase.txt" 2> /dev/null || print_error "Failed to write to /tmp/passphrase.txt"
			done
			debuild -k00582F84 -p"gpg --no-tty --batch --passphrase-fd 0" -S < /tmp/passphrase.txt && DEBUILD_RETVAL=$? || DEBUILD_RETVAL=$?
			rm -f /tmp/passphrase.txt

			if [ $DEBUILD_RETVAL -ne 0 ]; then
				print_warning "   debuild failed with status code ${DEBUILD_RETVAL}"
				continue
			fi
			cd ..

			DEBFILE="texworks_${DEB_VERSION}_source.changes"
			if [ -z "${TRAVIS_TAG}" ]; then
				PPA="ppa:texworks/ppa"
			else
				PPA="ppa:texworks/stable"
			fi
			print_info "   scheduling to upload ${DEBFILE} to ${PPA}"

			echo "dput \"${PPA}\" \"${BUILDDIR}/${DEBFILE}\"" >> "${TRAVIS_BUILD_DIR}/travis-ci/dput-launchpad.sh"
		done
	elif [ ${QT} -eq 5 ]; then
		print_info "Not packaging for ${TARGET_OS}/qt${QT}"
	else
		print_error "Skipping unsupported combination '${TARGET_OS}/qt${QT}'"
	fi
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
		echo_and_run "cp -r \"${TRAVIS_BUILD_DIR}/travis-ci/README.win\" \"package-zip/README.txt\""
		if [ ! -z "${TRAVIS_TAG}" -o ! -z "${FORCE_MANUAL}" ]; then
			print_info "Fetching manual"
			cd package-zip
			echo_and_run "python \"${TRAVIS_BUILD_DIR}/travis-ci/getManual.py\""
			cd ..
		fi

		print_info "Fetching poppler data"
		wget --no-check-certificate "${POPPLERDATA_URL}"
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
				"name": "TeXworks-for-Windows:latest",
				"repo": "Windows-latest",
				"subject": "texworks"
			},
			"version": {
				"name": "${VERSION_NAME}",
				"released": "${RELEASE_DATE}",
				"gpgSign": false
			},
			"files":
			[
				{"includePattern": "${BUILDDIR}/TeXworks-${TARGET_OS}-${VERSION_NAME}.zip", "uploadPattern": "TeXworks-${TARGET_OS}-${VERSION_NAME}.zip"}
			],
			"publish": true
		}
EOF
		if [ ! -z "${TRAVIS_TAG}" ]; then
			print_info "Preparing github-releases.txt"
			echo "${BUILDDIR}/TeXworks-${TARGET_OS}-${VERSION_NAME}.zip" > "${TRAVIS_BUILD_DIR}/travis-ci/github-releases.txt"
		fi
	else
		print_error "Skipping unsupported combination '${TARGET_OS}/qt${QT}'"
	fi
elif [ "${TARGET_OS}" = "osx" -a "${TRAVIS_OS_NAME}" = "osx" ]; then
	if [ ${QT} -eq 5 ]; then
		print_info "Running CPack"
		cpack --verbose

		print_info "Renaming .dmg"
		mv "${BUILDDIR}/"TeXworks.*.dmg "${BUILDDIR}/TeXworks-${TRAVIS_OS_NAME}-${VERSION_NAME}.dmg"

		print_info "Preparing bintray.json"
		cat > "${TRAVIS_BUILD_DIR}/travis-ci/bintray.json" <<EOF
		{
			"package": {
				"name": "TeXworks-for-Mac:latest",
				"repo": "OSX-latest",
				"subject": "texworks"
			},
			"version": {
				"name": "${VERSION_NAME}",
				"released": "${RELEASE_DATE}",
				"gpgSign": false
			},
			"files":
			[
				{"includePattern": "${BUILDDIR}/TeXworks-${TRAVIS_OS_NAME}-${VERSION_NAME}.dmg", "uploadPattern": "TeXworks-${TRAVIS_OS_NAME}-${VERSION_NAME}.dmg"}
			],
			"publish": true
		}
EOF
		if [ ! -z "${TRAVIS_TAG}" ]; then
			print_info "Preparing github-releases.txt"
			echo "${BUILDDIR}/TeXworks-${TARGET_OS}-${VERSION_NAME}.dmg" > "${TRAVIS_BUILD_DIR}/travis-ci/github-releases.txt"
		fi
	else
		print_error "Skipping unsupported combination '${TARGET_OS}/qt${QT}'"
	fi
else
	print_error "Skipping unsupported host/target combination '${TRAVIS_OS_NAME}/${TARGET_OS}'"
fi

cd "${TRAVIS_BUILD_DIR}"

print_info "Deployment preparation successful"
