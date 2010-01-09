// TeXworksScript
// Title: System test
// Description: (For demo purposes) Runs selection as a shell command, and replaces it with the output
// Author: Jonathan Kew
// Version: 0.3
// Date: 2010-01-09
// Script-Type: standalone
// Context: TeXDocument

var cmd = TW.target.selection;
if (cmd != "") {
  var result = TW.app.system(cmd);
  if (result != null) {
    TW.target.insertText(result);
  }
}
