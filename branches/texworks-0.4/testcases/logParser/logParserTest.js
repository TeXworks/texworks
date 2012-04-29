// TeXworksScript
// Title: Log parser tests
// Author: Antonio Macrì
// Version: 1.0
// Date: 2012-03-07
// Script-Type: standalone
// Context: TeXDocument
// Shortcut: Ctrl+K, Ctrl+K, Ctrl+T

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


justLoad = null;


function GenerateDiff(expected, unexpected)
{
  var s = "";
  s += "<table border='0' cellspacing='0' cellpadding='4'>";
  s += "<tr><td></td><td colspan='3'>Expected:</td></tr>";
  var k = 0;
  for (var i=0; i < expected.length; i++) {
    for (var j=k; j < unexpected.length; j++) {
      if (Result.Equals(expected[i], unexpected[j])) {
        var tmp = unexpected[k];
        unexpected[k] = unexpected[j];
        unexpected[j] = tmp;
        k++;
        break;
      }
    }
    if (j == unexpected.length) {
      s += LogParser.GenerateResultRow(expected[i]);
    }
  }
  if (k < unexpected.length) {
    s += "<tr><td></td><td colspan='3'>Unexpected:</td></tr>";
    for (; k<unexpected.length; k++) {
      s += LogParser.GenerateResultRow(unexpected[k]);
    }
  }
  s += "</table>";
  return s;
}


var file = TW.readFile("logParser.js");
if (file.status == 0) {
  eval(file.result);
  file = null;  // free mem

  var marker = "-----BEGIN OUTPUT BLOCK-----\n";

  var parser = new LogParser();
  var totalTests = 0;
  var failedTests = 0;

  var fex = [];
  fex[0] = function(){ return 2; };
  fex[1] = function(f) {
    var result = files.some(function(ff) {
      return f == ff || ff.slice(0,f.length) == f && (ff[f.length]=='\\'||ff[f.length]=='/');
    }) ? 0 : 1;
    return result;
  };

  var s = "";

  function RunTests(folder) {
    var grouping = false;
    for (var i = 1; ; i++) {
      var filename = folder + "/" + i + ".test";
      var result = TW.readFile(filename);
      if (result.status != 0) {
        break;
      }
      result = result.result;

      for (var j = 0; j < fex.length; j++) {
        TW.fileExists = fex[j];
        totalTests++;

        var index = result.indexOf(marker);
        var output = result.slice(index + marker.length);
        var exp = result.slice(0, index);
        parser.Parse(output);

        var expected = eval("(function(){return " + exp + ";})()");
        var generated = parser.Results;

        var passed = expected.length == generated.length;
        for (var k=0; k<expected.length && passed; k++) {
          passed = Result.Equals(expected[k], generated[k]);
        }

        if (!passed) {
          if (grouping) {
            s += "</td></tr>";
            grouping = false;
          }
          s += "<tr>";
          s += "<td style='background-color: red'></td>";
          s += "<td valign='top'>" + filename + " [" + j + "]</td>";
          s += "<td valign='top'><font size=-2>" + GenerateDiff(expected, generated) + "</font></td>";
          s += "</tr>";
        }
        else if (grouping) {
          s += ", " + filename + " [" + j + "]";
        }
        else {
          s += "<tr>";
          s += "<td style='background-color: green'></td>";
          s += "<td valign='top' colspan='2'>" + filename + " [" + j + "]";
          grouping = true;
        }
        failedTests += passed ? 0 : 1;
      }
    }
    if (grouping) {
      s += "</td></tr>";
    }
  }

  var folders = [ "tests-miktex", "tests-texlive-ubuntu" ];
  for (var j = 0; j < folders.length; j++) {
    var files = TW.readFile(folders[j] + "/files.js");
    if (files.status == 0) {
      files = eval("(function(){return " + files.result + ";})()");
    }
    else {
      files = [];
    }
    RunTests(folders[j]);
  }

  var html = "<html><body>";
  html += "Total tests: " + totalTests +
          ", Failed tests: " + failedTests + "<hr/>";
  html += "<table border='0' cellspacing='0' cellpadding='4'>";
  html += s;
  html += "</table></body></html>";
  TW.result = html;
}
else {
  TW.warning(null, "", "Cannot load script \"logParser.js\"!");
}
undefined;
