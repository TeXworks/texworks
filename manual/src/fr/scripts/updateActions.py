#!/usr/bin/python

SVNDIR="../../../trunk/"

from xml.etree import ElementTree

def makeLaTeXsafe(s):
	return s.replace("_", "\\_")

def format2Columns(items):
	ret = ""
	n = len(items)
	if n % 2 == 1: n += 1
	
	l = 0
	for i in range(n / 2):
		if len(makeLaTeXsafe(items[i])) > l: l = len(makeLaTeXsafe(items[i]))
	
	fmt = "{0:" + str(l + 1) + "}& "
	for i in range(n / 2):
		ret += fmt.format(makeLaTeXsafe(items[i]))
		if i + n / 2 < len(items):
			ret += makeLaTeXsafe(items[i + n / 2]) + " "
		ret += "\\\\\n"
	return ret


acts = []

def getActions(src, out):
	tree = ElementTree.parse(src)
	for action in tree.findall("//action"):
		if action.get("name"): acts.append(action.get("name"))
	
	menus = {}
	labels = {}
	# parse all menus
	for menu in tree.findall("//widget"):
		if menu.get("class") != "QMenu": continue
		name = menu.get("name")
		menus[name] = []
		for prop in menu.findall("property"):
			if prop.get("name") == "title": labels[name] = prop.find("string").text
		for act in menu.findall("addaction"):
			if act.get("name") and act.get("name") != "separator":
				menus[name].append(act.get("name"))
	
	# consolidate menus (add actions of submenus to parent menus)
	finished = False
	while not finished:
		finished = True
		for name in menus.keys():
			if name in menus:
				i = 0
				while i < len(menus[name]):
#					print name + " " + str(i) + " " + str(len(menus[name]))
					a = menus[name][i]
					if a in menus:
						del menus[name][i]
						menus[name][i:0] = menus[a]
						del menus[a]
						finished = False
					else:
						i += 1
	
	labels = labels.items()
	labels.sort(key = lambda x: x[1])
	f = open(out, 'w')
	f.write("\\begin{longtable}{QQ}\n")
	f.write("\\toprule\n")

	i = 0
	for (key, label) in labels:
		if not key in menus: continue
		if i > 0: f.write("%\n\\midrule\n%\n")
		f.write("\\multicolumn{2}{c}{" + makeLaTeXsafe(label) + "} \\\\\n")
		items = menus[key]
		items.sort()
		f.write(format2Columns(items))
		i += 1

	f.write("\\bottomrule\n")
	f.write("\\end{longtable}\n")
	f.close()
		
#	for menu in tree.findall("/widget/widget/widget"):
#		if menu.get("class") != "QMenu": continue
#		print menu.get("name")
#		for act in menu.findall("addaction"):
#			print "\t" + act.get("name")

#		break

getActions(SVNDIR + "src/TeXDocument.ui", "../menuactionsTeXDocument.tex")
getActions(SVNDIR + "src/PDFDocument.ui", "../menuactionsPDFDocument.tex")


# make unique & sort
acts = list(set(acts))
acts.sort()

f = open("../actionsAlphabetical.tex", 'w')
f.write("\\begin{longtable}{QQ}\n")
f.write("\\toprule\n")
f.write(format2Columns(acts))
f.write("\\bottomrule\n")
f.write("\\end{longtable}\n")

f.close()

