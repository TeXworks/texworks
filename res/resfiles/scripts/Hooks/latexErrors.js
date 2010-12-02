// TeXworksScript
// Title: LaTeX errors
// Description: Looks for errors in the LaTeX terminal output
// Author: Jonathan Kew & Stefan Löffler
// Version: 0.4
// Date: 2010-11-02
// Script-Type: hook
// Hook: AfterTypeset

// This is just a simple proof-of-concept; it will often get filenames wrong, for example.
// Switching the engines to use the FILE:LINE-style error messages could help a lot.

parenRE = new RegExp("[()]");
// Should catch filenames of the following forms:
// * ./abc, "./abc"
// * /abc, "/abc"
// * .\abc, ".\abc"
// * C:\abc, "C:\abc"
// * \\server\abc, "\\server\abc"
// Caveats: filenames with escaped " or space in the filename don't work (correctly)
newFileRE = new RegExp("^\\(\"?((?:\\./|/|.\\\\|[a-zA-Z]:\\\\|\\\\\\\\[^\\\" )]+\\\\)[^\" )]+)");
lineNumRE = new RegExp("^l\\.(\\d+)");
badLineRE = new RegExp("^(?:Over|Under)full \\\\hbox.*at lines (\\d+)");
warnLineRE = new RegExp("^(?:LaTeX|Package (?:.*)) Warning: .*");
warnLineNumRE = new RegExp("on input line (\\d+).");
errors = [];
warnings = [];
infos = [];

function trim (zeichenkette) {
  return zeichenkette.replace (/^\s+/, '').replace (/\s+$/, '');
}

// get the text from the standard console output
txt = TW.target.consoleOutput;
lines = txt.split('\n');

curFile = undefined;
filenames = [];
extraParens = 0;

for (i = 0; i < lines.length; ++i) {
	line = lines[i];
	
	// check for error messages
	if (line.match("^! ")) {
		var error = [];
		// record the current input file
		error[0] = curFile;
		// record the error message itself
		error[2] = line;
		// look ahead for the line number and record that
		error[1] = 0;
		while (++i < lines.length) {
			line = lines[i];
			if(trim(line) == '') break;
			matched = lineNumRE.exec(line);
			if (matched)
				error[1] = matched[1];
			error[2] += "\n" + line;
		}
		errors.push(error);
		continue;
	}
	
	// check for over- or underfull lines
	matched = badLineRE.exec(line);
	if (matched) {
		var error = [];
		error[0] = curFile;
		error[1] = matched[1];
		error[2] = line;
		infos.push(error);
		continue;
	}

	// check for other warnings
	matched = warnLineRE.exec(line);
	if (matched) {
		var error = [];
		error[0] = curFile;
		error[1] = "?";
		error[2] = line;

		while (++i < lines.length) {
			line = lines[i];
			if(line == '') break;
			error[2] += "\n" + line;
		}
		matched = warnLineNumRE.exec(error[2].replace(/\n/, ""));
		if (matched)
			error[1] = matched[1];
		warnings.push(error);
		continue;
	}

    // try to track beginning/ending of input files (flaky!)
	pos = line.search(parenRE);
	while (pos >= 0) {
		line = line.slice(pos);
		if (line.charAt(0) == ")") {
			if (extraParens > 0) {
				--extraParens;
			}
			else if (filenames.length > 0) {
				curFile = filenames.pop();
			}
			line = line.slice(1);
		}
		else {
			match = newFileRE.exec(line);
			if (match) {
				filenames.push(curFile);
				curFile = match[1];
				line = line.slice(match[0].length);
				extraParens = 0;
			}
			else {
				++extraParens;
				line = line.slice(1);
			}
		}
		if (line == undefined) {
			break;
		}
		pos = line.search(parenRE);
	}
}

function htmlize(str) {
	var html = str;
	html = html.replace(/&/g, "&amp;");
	html = html.replace(/</g, "&lt;");
	html = html.replace(/>/g, "&gt;");
	html = html.replace(/\n /g, "\n&nbsp;");
	html = html.replace(/  /g, "&nbsp;&nbsp;");
	html = html.replace(/&nbsp; /g, "&nbsp;&nbsp;");
	return html.replace(/\n/g, "<br />\n");
	
}

function makeResultRow(data, color) {
	var html = '';
	var url = 'texworks:' + data[0] + (data[1] != '?' && data[1] != 0 ? '#' + data[1] : '');
	html += '<tr>';
	html += '<td width="10" style="background-color: ' + color + '"></td>';
	html += '<td valign="top"><a href="' + url + '">' + data[0] + '</a></td>';
	html += '<td valign="top">' + data[1] + '</td>';
	html += '<td valign="top" style="font-family: monospace;">' + htmlize(data[2]) + '</td>';
	html += '</tr>';
	return html;
}

// finally, return our result (if any)
if (errors.length > 0 || warnings.length > 0 || infos.length > 0) {
	html  = '<html><body>';
	html += '<table border="1" cellspacing="0" cellpadding="4">';

	for(i = 0; i < errors.length; ++i)
		html += makeResultRow(errors[i], 'red');
	for(i = 0; i < warnings.length; ++i)
		html += makeResultRow(warnings[i], 'yellow');
	for(i = 0; i < infos.length; ++i)
		html += makeResultRow(infos[i], '#8080ff');

	html += "</table>";
	html += "</body></html>";
	TW.result = html;
}
undefined;
