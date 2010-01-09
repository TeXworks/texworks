// TeXworksScript
// Title: Babel language
// Description: Looks for a Babel line to set the spell-check language
// Author: Jonathan Kew
// Version: 0.3
// Date: 2010-01-09
// Script-Type: hook
// Hook: LoadFile

babelRE = new RegExp("\\\\usepackage\\[(?:.+,)*([^,]+)\\]\\{babel\\}");

spellingDict = new Array();

// extend or customize this list as needed
spellingDict.czech     = "cs_CZ";
spellingDict.german    = "de_DE";
spellingDict.germanb   = "de_DE";
spellingDict.ngerman   = "de_DE";
spellingDict.greek     = "el_GR";
spellingDict.english   = "en_US";
spellingDict.USenglish = "en_US";
spellingDict.american  = "en_US";
spellingDict.UKenglish = "en_GB";
spellingDict.british   = "en_GB";
spellingDict.spanish   = "es_ES";
spellingDict.french    = "fr_FR";
spellingDict.francais  = "fr_FR";
spellingDict.latin     = "la_LA";
spellingDict.latvian   = "lv_LV";
spellingDict.polish    = "pl_PL";
spellingDict.brazilian = "pt_BR";
spellingDict.brazil    = "pt_BR";
spellingDict.portuges  = "pt_PT";
spellingDict.portuguese= "pt_PT";
spellingDict.russian   = "ru_RU";
spellingDict.slovak    = "sk_SK";
spellingDict.slovene   = "sl_SL";
spellingDict.swedish   = "sv_SV";

// get the text from the document window
txt = TW.target.text;
lines = txt.split('\n');

// look for a babel line...
for (i = 0; i < lines.length; ++i) {
  line = lines[i];
  matched = babelRE.exec(line);
  if (matched) {
    lang = matched[1];
    if (spellingDict[lang]) {
      TW.target.setSpellcheckLanguage(spellingDict[lang]);
      TW.result = "Set spell-check language to " + spellingDict[lang];
    }
    break;
  }
  // ...but give up at the end of the preamble
  if (line.match("\\\\begin\\{document\\}")) {
    break;
  }
  if (line.match("\\\\starttext")) { // oops, seems to be ConTeXt!
    break;
  }
}
