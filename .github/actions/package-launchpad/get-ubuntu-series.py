import json, urllib.request

with urllib.request.urlopen('https://api.launchpad.net/1.0/ubuntu/series') as www:
	dat = json.loads(www.read())
print(' '.join([e['name'] for e in dat['entries'] if e['active']]))
