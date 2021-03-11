import base64, json, os, sys
import urllib.request
import urllib.error

repo = os.environ['ARTIFACTORY_REPO']
remotePath = os.environ['ARTIFACTORY_PATH']
baseurl = os.environ['ARTIFACTORY_URL']
username = os.environ['ARTIFACTORY_USER']
key = os.environ['ARTIFACTORY_KEY']

auth = 'Basic {0}'.format(base64.b64encode('{0}:{1}'.format(username, key).encode('utf-8')).decode('ascii'))

def sendRequest(r, quiet = False):
	try:
		r.add_header('Authorization', auth)
		with urllib.request.urlopen(r) as f:
			content = f.read()
		if not quiet:
			print('< Server replied')
			print(json.dumps(json.loads(content), sort_keys=True, indent=4))
		return content
	except urllib.error.HTTPError as e:
		print('< Server replied')
		print(e)
		sys.exit(1)

for path in sys.argv[1:]:
	filename = os.path.basename(path)

	with open(path, 'rb') as fin:
		print('> Uploading {0} as {1} to Artifactory ({2}/artifactory/{3}/{4})'.format(path, filename, baseurl, repo, remotePath))
		r = urllib.request.Request('{0}/artifactory/{1}/{2}/{3}'.format(baseurl, repo, remotePath, filename), data=fin, method='PUT')
		r.add_header('Content-Type', 'application/octet-stream')
		sendRequest(r)
