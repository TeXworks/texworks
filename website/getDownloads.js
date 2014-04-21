var downloads = [];
var macOSXCodenames = {'10.5': 'Leopard', '10.6': 'Snow Leopard', '10.7': 'Lion', '10.8' : 'Mountain Lion', '10.9' : 'Mavericks'};

function endsWith(str, suffix) {
	if (suffix.length > str.length)
		return false;
	return (str.substr(str.length - suffix.length) === suffix);
}

function humanReadableFilesize(filesize) {
	"use strict";
	var prefixes = ['', 'k', 'M', 'G', 'T'];
	var humanReadable = filesize;
	var i;

	for (i = 0; i < prefixes.length; i++) {
		if (humanReadable < 1000)
			break;
		humanReadable /= 1000;
	}
	// If we've run out of prefixes, i will have been incremented one additional time
	if (i >= prefixes.length)
		i = prefixes.length - 1;
	
	return Math.round(10 * humanReadable) / 10 + "&nbsp;" + prefixes[i] + "B";
}

function makeDownloadLink(download, label, versionRegExp) {
	"use strict";
	var html, m, info;
	html = '<a href="' + download.url + '" class="link">'; // TODO: safeguard
	html += label;
	info = [];
	
	m = download.filename.match(versionRegExp);
	if (m) {
		info[info.length] = 'version&nbsp;' + m[1];
	}
	info[info.length] = humanReadableFilesize(parseInt("" + download.size));
	if (info.length > 0) {
		html += '<div class="info">' + info.join(', ') + '</div>';
	}
	html += '</a>';
	return html;
}

function updateUi() {
	"use strict";
	var osType, osName, osVersion, userAgent, appVersion, i, j, m, html, codeName, el;
	var osVersions, osVersionIdx, downloadVersionIdx, downloadIdx;

	userAgent = navigator.userAgent;
	appVersion = navigator.appVersion;

///////////////////////////// DEBUG
//			appVersion = "Mac";
//			userAgent = "Mac OS X 10.8"
///////////////////////////// DEBUG

	if (appVersion.indexOf("Win") > -1) {
		osName = osType = "Windows";
	} else if (appVersion.indexOf("Mac") > -1) {
		osType = "Mac";
		osName = "Mac OS X";
		m = userAgent.match(/Mac OS X (\d+)(?:\.|_)(\d+)/);
		if (m && m.length >= 3) {
			osVersion = m[1] + "." + m[2];
		}
	} else {
		if (userAgent.toLowerCase().indexOf('ubuntu') > -1) {
			osType = "Linux";
			osName = "Ubuntu";
		} else if (userAgent.toLowerCase().indexOf('opensuse') > -1) {
			osType = "Linux";
			osName = "openSUSE";
		} else if (userAgent.toLowerCase().indexOf('debian') > -1) {
			osType = "Linux";
			osName = "Debian";
		} else if (navigator.platform.toLowerCase().indexOf('linux') > -1) {
			osType = "Linux";
		}
	}

	html = '';
	if (osType === "Windows") {
		// Find the installer
		for (i = 0; i < downloads.length; i++) {
			if (endsWith(downloads[i].filename, ".exe")) {
				html = makeDownloadLink(downloads[i], "Get TeXworks for Windows Installer", /^TeXworks.*?-(\d+\.\d+(?:\.\d)?)-r(\d+)/);
				break;
			}
		}
		// If no installer was found, default to the first file
		if (i >= downloads.length && downloads.length > 0) {
			html = makeDownloadLink(downloads[0], "Get TeXworks for Windows", /^TeXworks.*?-(\d+\.\d+(?:\.\d)?)-r(\d+)/);
		}
	}
	if (osType === "Mac") {
		// Find the download corresponding to the latest Mac OS X version <= osVersion
		osVersions = [];
		for (i in macOSXCodenames) {
			osVersions[osVersions.length] = i;
		}
		osVersionIdx = osVersions.indexOf(osVersion);

		downloadIdx = -1;
		downloadVersionIdx = -1;
		for (i = 0; i < downloads.length; i++) {
			m = downloads[i].filename.match(/^TeXworks-Mac-.*?-([^.-]+)\.dmg/);
			for (j = 0; j < osVersions.length; j++) {
				if (macOSXCodenames[osVersions[j]].replace(/\s/g, '').toLowerCase() == m[1].replace(/\s/g, '').toLowerCase()) {
					if (j > downloadVersionIdx && j <= osVersionIdx) {
						downloadIdx = i;
						downloadVersionIdx = j;
					}
					break;
				}
			}
		}
		
		if (downloadIdx > -1)
			html = makeDownloadLink(downloads[downloadIdx], "Get TeXworks for Mac&nbsp;OS&nbsp;X", /^TeXworks-Mac-((?:\d|\.)+)-/);
		// Note: There is no point in advertising a download for a Mac OS X
		// version >= the OS version.
	}
	if (osType === "Linux") {
		if (osName === 'Ubuntu') {
			html = '<a href="https://launchpad.net/~texworks/+archive/stable/" class="link">Get TeXworks for Ubuntu</a>';
		}
		if (osName === 'openSUSE') {
			html = '<a href="http://software.opensuse.org/search?q=texworks&baseproject=ALL&lang=en&exclude_debug=true" class="link">Get TeXworks for openSUSE</a>';
		}
		if (osName === 'Debian') {
			html = '<a href="http://packages.debian.org/de/sid/texworks" class="link">Get TeXworks for Debian</a>';
		}
	}

	if (html === '' && osType !== "Windows" && osType !== "Mac" && downloads.length > 0) {
		// Fallback: Sources
		html = makeDownloadLink(downloads[0], "Get TeXworks Sources", /texworks.*?-(\d+\.\d+(?:\.\d)?)-r(\d+)/);
	}


	if (html !== '') {
		if (osType === "Windows" || osType === "Mac") {
			html += '<div class="other_ways">Alternatively, your TeX distribution may offer a TeXworks package.</div>';
		}
		else if (osType === "Linux") {
			html += '<div class="other_ways">Alternatively, your Linux distribution may already offer a TeXworks package.</div>';
		}
	}
	else {
		// Final fallback: Redirect the user to Google Drive
		// (should only happen if the OS is recognized, but no suitable version
		// is found (e.g., when using an old, unsupported Mac OS X))
		html = '<a href="https://drive.google.com/folderview?id=0B5iVT8Q7W44pMkNLblFjUzdQUVE" class="link">Get TeXworks</a>';
	}
	html += '<div class="other_ways">Not what you are looking for? Check <a href="#Getting_TeXworks">Getting TeXworks</a> for other ways to obtain TeXworks.</div>';

	el = document.getElementById("tw_downloads");
	el.innerHTML = html;
	el.parentNode.style.display = 'block';
}


function receiveMessage(event) {
	"use strict";
	if (event.origin !== "http://www.gmodules.com") {
		return;
	}
	downloads = JSON.parse(event.data);
	updateUi();
}


top.addEventListener("message", receiveMessage, false);
updateUi();
