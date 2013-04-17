var downloads = [];
var macOSXCodenames = {'10.5': 'Leopard', '10.6': 'Snow Leopard', '10.7': 'Lion', '10.8' : 'Mountain Lion'};
var defaultMacVersion = 10.6;

function filter(haystack, key, value) {
	"use strict";
	var i, retVal;
	retVal = [];
	for (i = 0; i < haystack.length; i += 1) {
		if (haystack[i][key] === value) {
			retVal[retVal.length] = haystack[i];
		}
	}
	return retVal;
}

function arrayContains(haystack, needle) {
	"use strict";
	var i;
	for (i = 0; i < haystack.length; i += 1) {
		if (haystack[i] === needle) {
			return true;
		}
	}
	return false;
}

function makeGCDownloadLink(download, label, sizeRegExp) {
	"use strict";
	var html, m, info;
	html = '<a href="http:' + download.url + '" class="link">'; // TODO: safeguard
	html += label;
	info = [];
	m = download.url.match(sizeRegExp);
	if (m) {
		info[info.length] = 'version ' + m[1];
	}
	info[info.length] = download.size; // TODO: safeguard
	if (info.length > 0) {
		html += '<div class="info">' + info.join(', ') + '</div>';
	}
	html += '</a>';
	return html;
}

function updateUi() {
	"use strict";
	var osType, osName, osVersion, userAgent, appVersion, i, m, featuredDownloads, filteredDownloads, html, codeName, el;

	userAgent = navigator.userAgent;
	appVersion = navigator.appVersion;

///////////////////////////// DEBUG
//			appVersion = "Mac";
//			userAgent = "Mac"
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

	featuredDownloads = [];
	for (i = 0; i < downloads.length; i += 1) {
		if (arrayContains(downloads[i].labels, 'Featured')) {
			featuredDownloads[featuredDownloads.length] = downloads[i];
		}
	}

	html = '';
	if (osType === "Windows") {
		filteredDownloads = filter(featuredDownloads, 'opsys', 'Windows');
		if (filteredDownloads.length > 0) {
			html = makeGCDownloadLink(filteredDownloads[0], "Get TeXworks for Windows Installer", /\/TeXworks-setup-((?:\d|\.)+)-/);
		}
	}
	if (osType === "Mac") {
		filteredDownloads = filter(featuredDownloads, 'opsys', 'OSX');
		if (osVersion && macOSXCodenames[osVersion]) {
			// Try to find a matching package
			codeName = macOSXCodenames[osVersion].replace(/\s/g, '');
			for (i = 0; i < filteredDownloads.length; i += 1) {
				if (filteredDownloads[i].url.match(new RegExp('TeXworks-Mac-[^-]+-r\\d+-' + codeName + '\\.dmg'))) {
					html = makeGCDownloadLink(filteredDownloads[i], "Get TeXworks for Mac&nbsp;OS&nbsp;X " + osVersion + " (" + macOSXCodenames[osVersion] + ")", /\/TeXworks-Mac-((?:\d|\.)+)-/);
				}
			}
		}
		if (html === '') {
			// As fallback, use the default Mac version
			osVersion = defaultMacVersion;
			codeName = macOSXCodenames[osVersion].replace(/\s/g, '');
			for (i = 0; i < filteredDownloads.length; i += 1) {
				if (filteredDownloads[i].url.match(new RegExp('TeXworks-Mac-[^-]+-r\\d+-' + codeName + '\\.dmg'))) {
					html = makeGCDownloadLink(filteredDownloads[i], "Get TeXworks for Mac&nbsp;OS&nbsp;X " + osVersion + " (" + macOSXCodenames[osVersion] + ")", /\/TeXworks-Mac-((?:\d|\.)+)-/);
				}
			}
		}
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

	if (html === '') {
		// Fallback: Sources
		filteredDownloads = filter(featuredDownloads, 'type', 'Source');
		if (filteredDownloads.length > 0) {
			html = makeGCDownloadLink(filteredDownloads[0], "Get TeXworks Sources", /\/texworks-((?:\d|\.)+)-/);
		}
	}

	if (osType === "Windows" || osType === "Mac") {
		html += '<div class="other_ways">Alternatively, your TeX distribution may offer a TeXworks package.</div>';
	}
	else if (osType === "Linux") {
		html += '<div class="other_ways">Alternatively, your Linux distribution may already offer a TeXworks package.</div>';
	}

	if (html !== '') {
		html += '<div class="other_ways">Not what you are looking for? Check <a href="#Getting_TeXworks">Getting TeXworks</a> for other ways to obtain TeXworks.</div>';

		el = document.getElementById("tw_downloads");
		el.innerHTML = html;
		el.parentNode.style.display = 'block';
	}
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
