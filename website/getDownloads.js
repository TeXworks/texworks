var osType, osName, osVersion, userAgent, appVersion, folderId;
var downloads = [];
var macOSXCodenames = {'10.5': 'Leopard', '10.6': 'Snow Leopard', '10.7': 'Lion', '10.8' : 'Mountain Lion', '10.9' : 'Mavericks', '10.10' : 'Yosemite'};

userAgent = navigator.userAgent;
appVersion = navigator.appVersion;

///////////////////////////// DEBUG
//            appVersion = "Mac";
//            userAgent = "Mac OS X 10.8"
///////////////////////////// DEBUG

if (appVersion.indexOf("Win") > -1) {
    osName = osType = "Windows";
} else if (appVersion.indexOf("Mac") > -1) {
    osType = "Mac";
    osName = "Mac OS X";
    var m = userAgent.match(/Mac OS X (\d+)(?:\.|_)(\d+)/);
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
    } else if (userAgent.toLowerCase().indexOf('fedora') > -1) {
        osType = "Linux";
        osName = "Fedora";
    } else if (navigator.platform.toLowerCase().indexOf('linux') > -1) {
        osType = "Linux";
    }
}

///////////////////////////// DEBUG
// osType = "Windows";
// osName = "Mac OS X";
// osVersion = "10.10";
///////////////////////////// DEBUG

// Default: sources
folderId = '0B5iVT8Q7W44pNWNZd1VSaWNCUEU';

if (osType === "Windows") {
    folderId = '0B5iVT8Q7W44pYzBwMjFBWGdVVHM';
} else if (osType === "Mac") {
    folderId = '0B5iVT8Q7W44pMTY5YjNzZmdzS1U';
} else if (osType === "Linux" && osName !== null) {
    // For some Linux distros, we link to third-party packages
    folderId = null;
}


function Download() {
    "use strict";
    this.id = "";
    this.mimetype = "";
    this.filename = "";
    this.size = null;
    this.url = "";
}


function endsWith(str, suffix) {
    "use strict";
    if (suffix.length > str.length) { return false; }
    return (str.substr(str.length - suffix.length) === suffix);
}

function humanReadableFilesize(filesize) {
    "use strict";
    var prefixes = ['', 'k', 'M', 'G', 'T'], humanReadable = filesize, i;

    for (i = 0; i < prefixes.length; i++) {
        if (humanReadable < 1000) { break; }
        humanReadable /= 1000;
    }
    // If we've run out of prefixes, i will have been incremented one additional time
    if (i >= prefixes.length) { i = prefixes.length - 1; }

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
    if (download.size) {
        info[info.length] = humanReadableFilesize(parseInt(String(download.size), 10));
    }
    if (info.length > 0) {
        html += '<div class="info">' + info.join(', ') + '</div>';
    }
    html += '</a>';
    return html;
}

function updateUi() {
    "use strict";
    var i, j, m, html, el, osVersions, osVersionIdx, downloadVersionIdx, downloadIdx;

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
            for (i = 0; i < downloads.length; i++) {
                if (endsWith(downloads[i].filename, ".zip")) {
                    html = makeDownloadLink(downloads[0], "Get TeXworks for Windows", /^TeXworks.*?-(\d+\.\d+(?:\.\d)?)-r(\d+)/);
                    break;
                }
            }
        }
    }
    if (osType === "Mac") {
        // Find the download corresponding to the latest Mac OS X version <= osVersion
        osVersions = [];
        for (i in macOSXCodenames) {
            // check that i is an actual member of macOSXCodenames we want (not, e.g., a member of its prototype, a constructor, etc.)
            if (typeof macOSXCodenames[i] === "string") { osVersions[osVersions.length] = i; }
        }
        osVersionIdx = osVersions.indexOf(osVersion);

        downloadIdx = -1;
        downloadVersionIdx = -1;
        for (i = 0; i < downloads.length; i++) {
            m = downloads[i].filename.match(/^TeXworks-Mac-.*?-([^\-.]+)\.dmg/);
            if (!m) {
                continue;
            }
            for (j = 0; j < osVersions.length; j++) {
                if (macOSXCodenames[osVersions[j]].replace(/\s/g, '').toLowerCase() === m[1].replace(/\s/g, '').toLowerCase()) {
                    if (j > downloadVersionIdx && j <= osVersionIdx) {
                        downloadIdx = i;
                        downloadVersionIdx = j;
                    }
                    break;
                }
            }
        }

        if (downloadIdx > -1) {
            html = makeDownloadLink(downloads[downloadIdx], "Get TeXworks for Mac&nbsp;OS&nbsp;X", /^TeXworks-Mac-((?:\d|\.)+)-/);
        }
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
        if (osName === 'Fedora') {
            html = '<a href="https://admin.fedoraproject.org/pkgdb/package/texworks/" class="link">Get TeXworks for Fedora</a>';
        }
    }

    if (html === '' && osType !== "Windows" && osType !== "Mac" && downloads.length > 0) {
        // Fallback: Sources
        html = makeDownloadLink(downloads[0], "Get TeXworks Sources", /texworks.*?-(\d+\.\d+(?:\.\d)?)-r(\d+)/);
    }


    if (html !== '') {
        if (osType === "Windows" || osType === "Mac") {
            html += '<div class="other_ways">Alternatively, your TeX distribution may offer a TeXworks package.</div>';
        } else if (osType === "Linux") {
            html += '<div class="other_ways">Alternatively, your Linux distribution may already offer a TeXworks package.</div>';
        }
    } else {
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

function parseFolder(data) {
    "use strict";
    var reGlob, re, m, mm, i, d;
    reGlob = /<script>insertIcon\('[^']*', '([^']*)'\)<\/script><\/div><div class="folder-row-inner"><div class="folder-cell"><a href="([^"]*)">([^<]*)<\/a><\/div>/g;
    re = /<script>insertIcon\('[^']*', '([^']*)'\)<\/script><\/div><div class="folder-row-inner"><div class="folder-cell"><a href="([^"]*)">([^<]*)<\/a><\/div>/;

    // Clear downloads
    downloads = [];

    // Find beginning of main data
    data = data.substr(data.indexOf("<body>"));
    // Find strings of interest
    m = data.match(reGlob);
    for (i = 0; i < m.length; i++) {
        // Parse string
        mm = re.exec(m[i]);
        if (!mm || mm.length < 4) {
            continue; // this should never happen as the RegExp has matched before
        }
        // Ignore folders
        if (mm[1] === 'application/vnd.google-apps.folder') {
            continue;
        }
        d = new Download();
        d.mimetype = mm[1];
        d.url = "https://googledrive.com" + mm[2];
        d.filename = mm[3];
        downloads[downloads.length] = d;
    }
    updateUi();
}

if (folderId) {
    fetchFolder(folderId).done(function (data) { "use strict"; parseFolder(data); }).fail(function () { "use strict"; updateUi(); });
} else {
    updateUi();
}
