#!/usr/bin/python

SVNDIR="../../../trunk/"

from xml.etree import ElementTree


def makeLaTeXsafe(s):
	return s.replace("\\", "\\textbackslash ").replace("_", "\\_").replace("{", "\\{").replace("}", "\\}").replace("&", "\\&").replace("#RET#", "{\\AutoCompRet}").replace("#INS#", "{\\AutoCompIns}").replace("#", "\\#").replace("-", "{-}")

def autocompletionToFile(src, out):
	f = open(src, 'r')
	rules = {}
	for line in f:
		if line[0] == '%': continue
		line = line.strip().split(':=')
		if len(line) == 2:
			(key, value) = line
		else:
			key = value = line[0]
		rules[key] = value
	f.close()
	
	items = []
	n1 = n2 = 0
	for key in rules.keys():
		if not key in rules: continue
		v1 = v2 = ''
		v3 = rules[key]
		if key == v3:
			pass
		elif key[0] == '\\':
			v2 = key
			if key[1:] in rules and rules[key[1:]] == v3:
				v1 = key[1:]
				del rules[key[1:]]
		else:
			v1 = key
			if "\\" + key in rules and rules["\\" + key] == v3:
				v2 = "\\" + key
				del rules["\\" + key]
		del rules[key]
		if len(makeLaTeXsafe(v1)) > n1: n1 = len(makeLaTeXsafe(v1))
		if len(makeLaTeXsafe(v2)) > n2: n2 = len(makeLaTeXsafe(v2))
		items.append((v1, v2, v3))
	
	items.sort(cmp = lambda x, y: cmp(x[2].lower(), y[2].lower()))
	fmt = "{0:" + str(n1) + "} & {1:" + str(n2) + "} & {2} \\\\\n"
	f = open(out, 'w')

	f.write("\\begin{longtable}{>{\\footnotesize}p{15mm}>{\\footnotesize}p{15mm}>{\\footnotesize}p{95mm}}\n")
	f.write("\\toprule\n")
#	f.write("Shortcut & Action \\\\\n")
#	f.write("\\midrule \\endhead\n")

	for (v1, v2, v3) in items:
		f.write(fmt.format(makeLaTeXsafe(v1), makeLaTeXsafe(v2), makeLaTeXsafe(v3)))

	f.write("\\bottomrule\n")
	f.write("\\end{longtable}\n")
	f.close()


#shortcutsToFile(SVNDIR + "src/TeXDocument.ui", "../shortcutsTeXDocument.tex")
#shortcutsToFile(SVNDIR + "src/PDFDocument.ui", "../shortcutsPDFDocument.tex")
autocompletionToFile(SVNDIR + "res/resfiles/completion/tw-basic.txt", "../autocompletionBasic.tex")
autocompletionToFile(SVNDIR + "res/resfiles/completion/tw-latex.txt", "../autocompletionLatex.tex")

