import subprocess, os, os.path, sys, re, shutil

#OBJDUMP = '/opt/mxe/usr/bin/i686-w64-mingw32.shared-objdump'
#BASEDIR = '/opt/mxe/usr/i686-w64-mingw32.shared/bin/'
OBJDUMP = 'objdump'
BASEDIR = 'c:/msys64/mingw64/bin'

def getDependencies(filename):
	out = subprocess.check_output([OBJDUMP, '-x', filename], universal_newlines = True)
	return set(re.findall('DLL Name: (.*)', out))

def getDependenciesRecursively(filename, checkedAlready = None):
	rv = checkedAlready if checkedAlready is not None else set()
	for dep in getDependencies(filename):
		if dep in rv: continue

		rv.add(dep)
		filename = os.path.join(BASEDIR, dep)
		if not os.path.exists(filename): continue

		rv = rv.union(getDependenciesRecursively(filename, rv))
	return rv



if len(sys.argv) != 2:
	print('Usage: %s file.exe' % sys.argv[0])
	sys.exit(1)

print('Checking dependencies for %s' % sys.argv[1])

OUTDIR = os.path.dirname(sys.argv[1])
# Replace an empty OUTDIR by '.' to have a nicer output
if OUTDIR == '': OUTDIR = '.'

for dep in sorted(getDependenciesRecursively(sys.argv[1])):
	src = os.path.normpath(os.path.join(BASEDIR, dep))
	dst = os.path.normpath(os.path.join(OUTDIR, dep))

	if os.path.exists(dst):
		print('Skipping %s - already in %s' % (dep, OUTDIR))
		continue
	if not os.path.exists(src):
		print('Skipping %s - not in %s' % (dep, BASEDIR))
		continue
	print('%s > %s' % (src, dst))
	shutil.copy(src, dst)
