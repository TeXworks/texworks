#!/usr/bin/python

SVNDIR="../../../trunk/"

from xml.etree import ElementTree


def shortcutsToFile(src, out):
	res = []
	l = 0

	tree = ElementTree.parse(src)
	for action in tree.findall("//action"):
		name = None
		key = None
		for prop in action.findall("property"):
			if prop.get("name") == "text":
				name = prop.find("string").text
			if prop.get("name") == "shortcut":
				key = prop.find("string").text
		if name and key:
			res.append((key, name))
			if len(key) > l: l = len(key)

	res.sort(key = lambda x: x[0])

	fmt = "{0:" + str(l + 1) + "}& {1} \\\\\n"
	
	f = open(out, 'w')
	f.write("\\begin{longtable}{Pl}\n")
	f.write("\\toprule\n")
	f.write("Shortcut & Action \\\\\n")
	f.write("\\midrule \\endhead\n")

	for (key, name) in res:
		f.write(fmt.format(key, name))

	f.write("\\bottomrule\n")
	f.write("\\end{longtable}\n")
	f.close()

shortcutsToFile(SVNDIR + "src/TeXDocument.ui", "../shortcutsTeXDocument.tex")
shortcutsToFile(SVNDIR + "src/PDFDocument.ui", "../shortcutsPDFDocument.tex")

