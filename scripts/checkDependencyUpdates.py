#!/usr/bin/env python3
import re, urllib.request

# https://stackoverflow.com/a/4836734
def natural_sort(l):
	convert = lambda text: int(text) if text.isdigit() else text.lower()
	alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
	return sorted(l, key = alphanum_key)

class Package:
	def __init__(self, name, pkgListUrl = None, pkgRegex = None, downloadUrl = None, versionRegex = r'([0-9.]+[0-9])'):
		self.name = name
		self.pkgListUrl = pkgListUrl
		self.pkgRegex = pkgRegex
		self.versionRegex = versionRegex
		self._versionsInitialized = False
		self.downloadUrl = downloadUrl

	def __repr__(self):
		return 'Package<{}>'.format(self.name)

	def getVersions(self):
		if self._versionsInitialized: return
		# At least ijg.org requires a user-agent to be set
		r = urllib.request.Request(self.pkgListUrl, headers = {'User-Agent': 'python'})
		with urllib.request.urlopen(r) as fin:
			pkgList = re.findall(self.pkgRegex, fin.read().decode())
		pkgList = list(set(pkgList))
		self.pkgVersions = {}
		for pkg in pkgList:
			m = re.search(self.versionRegex, pkg)
			self.pkgVersions[m.group(1)] = pkg
		self._versionsInitialized = True

	def getLatestVersion(self):
		self.getVersions()
		return natural_sort(self.pkgVersions)[-1]

	def getDownloadUrl(self, version):
		if self.downloadUrl is None:
			self.getVersions()
			return self.pkgListUrl + self.pkgVersions[version]
		if callable(self.downloadUrl):
			return self.downloadUrl(version)
		return self.downloadUrl.format(version)

	def getLatestDownloadUrl(self):
		return self.getDownloadUrl(self.getLatestVersion())

class GithubPackage(Package):
	def __init__(self, name, githubProject, tagFormat = 'v{}', versionRegex = '([0-9.]+[0-9])'):
		Package.__init__(self, name,
			pkgListUrl = 'https://github.com/{}/releases'.format(githubProject),
			pkgRegex = r'/releases/expanded_assets/' + tagFormat.format(versionRegex),
			downloadUrl = 'https://github.com/{}/archive/refs/tags/{}.tar.gz'.format(githubProject, tagFormat),
			versionRegex = versionRegex)

# Define all packages used by TeXworks (on Windows and/or macOS)
pkgs = dict([(p.name, p) for p in [
	Package('fontconfig', 'https://www.freedesktop.org/software/fontconfig/release/', r'fontconfig-[0-9.]+\.tar\.xz'),
	Package('freetype', 'https://download.savannah.gnu.org/releases/freetype/', r'freetype-[0-9.]+\.tar\.xz', lambda v: 'https://github.com/freetype/freetype/archive/refs/tags/VER-{}.tar.gz'.format(v.replace('.', '-'))),
	Package('gettext', 'https://ftp.gnu.org/gnu/gettext/', r'gettext-[0-9.]+\.tar\.xz'),
	GithubPackage('hunspell', 'hunspell/hunspell'),
	GithubPackage('lcms2', 'mm2/Little-CMS', tagFormat = 'lcms{}'),
	Package('libjpeg', 'https://ijg.org/files/', r'jpegsrc.v[0-9.a-zA-Z]+\.tar\.gz', versionRegex = r'(?<=\.v)([0-9.a-zA-Z]+)(?=\.tar\.gz)'),
	GithubPackage('libopenjpeg', 'uclouvain/openjpeg'),
	Package('libpng', 'http://www.libpng.org/pub/png/libpng.html', r'libpng-[0-9.]+\.tar\.xz', 'https://github.com/glennrp/libpng/archive/refs/tags/v{}.tar.gz'),
	Package('libtiff', 'https://download.osgeo.org/libtiff/', r'tiff-[0-9.]+\.tar\.gz'),
	Package('lua', 'https://www.lua.org/ftp/', r'lua-[0-9.]+\.tar\.gz'),
	Package('poppler', 'https://poppler.freedesktop.org/', r'poppler-[0-9.]+\.tar\.xz'),
	Package('poppler-data', 'https://poppler.freedesktop.org/', r'poppler-data-[0-9.]+\.tar\.gz'),
	Package('zlib', 'http://zlib.net/', r'zlib-[0-9.]+\.tar\.xz', 'https://github.com/madler/zlib/archive/refs/tags/v{}.tar.gz"'),
]])

# Load CMake files for obtaining/building the dependencies
with open('../.github/actions/msvc-dependencies/CMakeLists.txt') as fin:
	msvc = fin.read()
with open('../.github/actions/setup-macos/CMakeLists.txt') as fin:
	macos = fin.read()
with open('../CMake/packaging/CMakeLists.txt') as fin:
	packaging = fin.read()
with open('../.github/workflows/Dockerfile.appimage-debian') as fin:
	appimage = fin.read()

# Print header
maxNameLen = max([len(n) for n in pkgs])
print('{{:{len}s}}  MSVC    macOS     pkg     appImg'.format(len = maxNameLen).format(''))

# Print version information for each package and each CMake file
for name in pkgs:
	print('{{:{len}s}}'.format(len = maxNameLen).format(name), end = '', flush = True)
	url = pkgs[name].getLatestDownloadUrl()
	for haystack in [msvc, macos, packaging, appimage]:
		# Note: if the package name does not appear in the CMake file, the
		# package likely isn't used. This is a bit flaky in both directions
		# (a package "lcms2" might only be referenced by url
		# https://some.mirror.org/v1.2.3.tar.gz without the string "lcms2";
		# similarly, if only "poppler-data" is used, the package "poppler" would
		# be erroneously recognized as well).
		# For the most part, it happens to work sufficiently, though.
		if not name in haystack:
			print(u' {:^6s} '.format('---'), end = '')
		elif url in haystack:
			print(u'\u001b[32m {:^6s} \u001b[0m'.format('OK'), end = '')
		else:
			print(u'\u001b[93;1m {:^6s} \u001b[0m'.format('check'), end = '')
		print(' ', end = '')
	print(' {:7s} {}'.format(pkgs[name].getLatestVersion(), url))
