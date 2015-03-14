# Summary #

TeXworks uses the standard Qt mechanisms to support translating the user interface into other languages than the default (built-in) English. This page is intended to help would-be translators get started. For more details, see the Qt Linguist documentation (included in Qt Assistant), which you should have if you've installed a [current version of Qt4](http://trolltech.com/downloads/opensource#qt-open-source-edition).

In brief, to provide a translation for a particular language, you need to
  * use the `lupdate` tool to create a `.ts` file, which is an XML file containing all the text strings from the user interface;
  * use Linguist to edit this file, adding the translation for each string;
  * use Linguist or `lrelease` to compile the `.ts` file into a `.qm` file that can be loaded at runtime; and
  * put this file into the TeXworks `translations` folder.

# A little more detail #

Translation files are named with a two-letter language code such as "de" (German), "fr" (French), etc., following the conventions for locale names. A country code can be added where there are variations between different languages (e.g., "en\_US" versus "en\_GB"), but in most cases it should not be necessary to make such distinctions.

To create a new `.ts` file for a particular language, run `lupdate` on the `.pro` file, and specify the `.ts` file to be created (in the `trans` directory). We'll use German as an example:

```
    lupdate TeXworks.pro -ts trans/TeXworks_de.ts
```

(On some distributions, particularly if both Qt3 and Qt4 are installed, the tool may be called `lupdate-qt4`.)

The same command is used to _update_ the `.ts` file when there are changes in the TeXworks source. This allows you to keep all your existing translation work, and just add or modify strings for any changed parts of the user interface.

Use Qt Linguist to edit the `.ts` file, adding translations. Note that Linguist can show the source code where the string is used, which may be helpful in understanding the context if it is not immediately clear. It also "remembers" your translations, so that if the same string occurs again, it can suggest the same translation.

Finally, use the File/Release command within Linguist to generate a compiled `.qm` file (or use the `lrelease` command-line tool). Note that you can do this at any stage; if you have not translated all the strings, the original English will be used for those that are missing. So you can try out your work before finishing it all.

For your translation to be loaded by TeXworks, you need to put the `.qm` file in the `translations` folder (in the TeXworks resources folder, alongside others such as templates and completion files); then re-start the TeXworks application. Note that simply including it in the TeXworks **source** directory and rebuilding the program will not currently make it available at runtime; translation files are separate from the main application.

I welcome contributions of `.ts` files to be included in the TeXworks source, and eventually bundled with the program when it ships.