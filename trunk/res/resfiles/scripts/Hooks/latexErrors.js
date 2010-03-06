// TeXworksScript
// Title: LaTeX errors
// Description: Looks for errors in the LaTeX terminal output
// Author: Jonathan Kew & Stefan Löffler
// Version: 0.3
// Date: 2010-01-09
// Script-Type: hook
// Hook: AfterTypeset

// This is just a simple proof-of-concept; it will often get filenames wrong, for example.
// Switching the engines to use the FILE:LINE-style error messages could help a lot.

parenRE = new RegExp("[()]");
newFileRE = new RegExp("^\\(([\\./][^ )]+)");
lineNumRE = new RegExp("^l\\.(\\d+)");
badLineRE = new RegExp("^(?:Over|Under)full \\\\hbox.*at lines (\\d+)");
warnLineRE = new RegExp("^(?:LaTeX|Package (?:.*)) Warning: .*");
warnLineNumRE = new RegExp("on input line (\\d+).");
result = [];

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
			matched = lineNumRE.exec(line);
			if (matched) {
				error[1] = matched[1];
				break;
			}
		}
		result.push(error);
		continue;
	}
	
	// check for over- or underfull lines
	matched = badLineRE.exec(line);
	if (matched) {
		var error = [];
		error[0] = curFile;
		error[1] = matched[1];
		error[2] = line;
		result.push(error);
		continue;
	}

	// check for other warnings
	matched = warnLineRE.exec(line);
	if (matched) {
		var error = [];
		error[0] = curFile;
		error[1] = "?";
		matched = warnLineNumRE.exec(line);
		if (matched)
			error[1] = matched[1];
		error[2] = "";
		while (line != "" && i < lines.length) {
			error[2] += line;
			i++;
			line = lines[i];
		}
		result.push(error);
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

// finally, return our result (if any)
if (result.length > 0) {
	TW.result = result;
}
