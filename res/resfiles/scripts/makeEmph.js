// TeXworksScript
// Title: Make Emphasized
// Description: Encloses the current selection in \textem{}
// Author: Jonathan Kew
// Version: 0.1
// Date: 2009-08-31
// Script-Type: standalone

var txt = target.selection;
var len = txt.length;
target.insertText("\\textem{" + txt + "}");
target.selectRange(target.selectionStart - len - 1, len);
