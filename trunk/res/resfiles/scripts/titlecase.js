// TeXworksScript
// Title: Title Case
// Description: Converts the Current Selection to Title Case
// Author: Jonathan Kew
// Version: 0.1
// Date: 2009-09-07
// Script-Type: standalone
// Context: TeXDocument

/* To Title Case 1.1.1
 * David Gouch <http://individed.com>
 * 23 May 2008
 *
 * Copyright (c) 2008 David Gouch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* MODIFIED by Jonathan Kew for use with TeXworks: added optional leading
   backslash to the word-finding regex, to protect TeX control words */

String.prototype.toTitleCase = function() {
    return this.replace(/(\\?[\w&`'��"�.@:\/\{\(\[<>_]+-? *)/g, function(match, p1, index, title) {
        if (index > 0 && title.charAt(index - 2) !== ":" &&
        	match.search(/^(a(nd?|s|t)?|b(ut|y)|en|for|i[fn]|o[fnr]|t(he|o)|vs?\.?|via)[ \-]/i) > -1)
            return match.toLowerCase();
        if (title.substring(index - 1, index + 1).search(/['"_{(\[]/) > -1)
            return match.charAt(0) + match.charAt(1).toUpperCase() + match.substr(2);
        if (match.substr(1).search(/[A-Z]+|&|[\w]+[._][\w]+/) > -1 || 
        	title.substring(index - 1, index + 1).search(/[\])}]/) > -1)
            return match;
        return match.charAt(0).toUpperCase() + match.substr(1);
    });
};

// thanks to David Gouch's function, the actual TW script is trivial:
if (target.objectName == "TeXDocument") {
  var txt = target.selection;
  if (txt != "") {
    var pos = target.selectionStart;
    txt = txt.toTitleCase();
    target.insertText(txt);
    target.selectRange(pos, txt.length);
  }
} else {
  "This script only works in source document windows."
}
