This page lists a couple of coding projects that are in our queue. This list is not necessarily exhaustive, but gives some reasonably well-defined tasks that should typically not interfere too much with other tasks.

If you are interested to help, browse this list and pick a task you are interested in and feeling up to. Please drop [us](http://code.google.com/p/texworks/people/list) a line or post to the [mailing list](http://tug.org/mailman/listinfo/texworks) before launching into coding to avoid problems down the road.

Note that:
  * most (if not all) projects below require some knowledge of C++, and
  * you need to be able to build TeXworks from source. See [Building](Building.md) for instruction to get started.

If you don't have experience with C++/programming but want to help the project in other ways, please have a look at the [TeXworks homepage](http://www.tug.org/texworks/#How_can_you_help) for other ways to get involved.

_Note: Please don't add your own ideas in the comments section. Instead, open a new issue or drop us a note on the [mailing list](http://tug.org/mailman/listinfo/texworks) to keep this page organized._



### Dialog to insert control data ###
TeXworks can be controlled by special "%!TEX" [control strings](TipsAndTricks.md) at the beginning of the file, yet it lacks an easy UI to add/edit/remove them.

  * Difficulty: easy
  * Time: short
  * Prerequisites: (C++, Qt) _or_ QtScript
  * Related issues: [issue 70](https://code.google.com/p/texworks/issues/detail?id=70)


### SyncTeX module ###
Currently, SyncTeX is integrated tightly into the editor and previewer code. For this project, it should be refactored into a module of its own with a well-defined interface to the editor and previewer. That would also make it possible to sync with external programs.

  * Difficulty: easy-intermediate
  * Time: short
  * Prerequisites: C++
  * Related issues: [issue 36](https://code.google.com/p/texworks/issues/detail?id=36), [issue 188](https://code.google.com/p/texworks/issues/detail?id=188), [issue 412](https://code.google.com/p/texworks/issues/detail?id=412)

### Spellchecker module ###
Currently, the spellchecker [Hunspell](http://hunspell.sourceforge.net/) is integrated tightly into the editor (and some other parts of the code). The purpose of this project is to refactor it into a module of its own with a well-defined interface to the editor. This makes it easy to support other/multiple spell checking backends (e.g., [Enchant](http://www.abisource.com/projects/enchant/)) or to add other nifty features (such as multiple dictionaries, a spell checking dialog, etc.).

  * Difficulty: easy-intermediate
  * Time: short
  * Prerequisites: C++
  * Related issues: [issue 74](https://code.google.com/p/texworks/issues/detail?id=74), [issue 90](https://code.google.com/p/texworks/issues/detail?id=90), [issue 227](https://code.google.com/p/texworks/issues/detail?id=227), [issue 247](https://code.google.com/p/texworks/issues/detail?id=247), [issue 251](https://code.google.com/p/texworks/issues/detail?id=251)

### Syntax highlighting module ###
Currently, the syntax highlighting module is tightly integrated into the editor (and some other parts of the code). The goal of this project is to refactor it into a module of its own with a well-defined interface to the editor (which should be generic enough to allow other highlighters, such as the spellchecker, to use it as well).

  * Difficulty: easy-intermediate
  * Time: short
  * Prerequisites: C++

### Autocompletion module ###
Currently, autocompletion is tightly integrated into the editor (and some other parts of the code). The aim of this project is to refactor it into a module of its own with a well-defined interface to the editor (which should be generic enough to allow other context-sensitive suggestion-providing components, such as the spell checker, to use it as well). This should also include "code hinting" (i.e., a drop down list popping up during typing).

  * Difficulty: intermediate
  * Time: short-medium
  * Prerequisites: C++, Qt
  * Related issues: [issue 101](https://code.google.com/p/texworks/issues/detail?id=101), [issue 136](https://code.google.com/p/texworks/issues/detail?id=136), [issue 284](https://code.google.com/p/texworks/issues/detail?id=284), [issue 285](https://code.google.com/p/texworks/issues/detail?id=285)


### Enhanced syntax highlighting ###
The current syntax highlighting definition is based exclusively on regular expressions. This prevents certain often-needed features, such as highlighting everything between matching pairs of parentheses. The purpose of this project is to implement extended syntax highlighting patterns (using, e.g., a combination of regular expressions and a self-written parser).

  * Difficulty: intermediate
  * Time: short-medium
  * Prerequisites: C++, Qt
  * Related issues: [issue 276](https://code.google.com/p/texworks/issues/detail?id=276), [issue 402](https://code.google.com/p/texworks/issues/detail?id=402), [issue 533](https://code.google.com/p/texworks/issues/detail?id=533)


### Keyboard shortcut manager ###
TeXworks supports keyboard shortcuts for a wide variety of functions, including scripts. There currently is no easy way (i.e., no GUI) to view them or customize them. A good starter could be the Qt source code (Qt Designer includes a keyboard definition GUI widget).

  * Difficulty: intermediate-challenging
  * Time: medium
  * Prerequisites: C++, Qt
  * Related issues: [issue 17](https://code.google.com/p/texworks/issues/detail?id=17), [issue 412](https://code.google.com/p/texworks/issues/detail?id=412), [issue 432](https://code.google.com/p/texworks/issues/detail?id=432), [issue 434](https://code.google.com/p/texworks/issues/detail?id=434)

### Per-script permissions ###
Currently, permissions and security settings can only be applied on a global basis for all scripts. The goal of this project is to allow script-specific overrides. Since this tackles security issues, a lot of planning and design is required before actually starting coding.

  * Difficulty: intermediate-challenging
  * Time: medium
  * Prerequisites: C++
  * Related issues: [issue 359](https://code.google.com/p/texworks/issues/detail?id=359)

### Automatic charset detection ###
TeXworks supports a variety of encodings through Qt, but not method is currently available to guesstimate the correct encoding when loading a file. Instead, choosing the correct encoding is left to the user, which is not "lowering the entry barrier". Most likely, this project will be implemented on top of an existing library, such as a [Mozilla Charset Detector](http://kildare.stage.mozilla.com/projects/intl/chardet.html).

  * Difficulty: intermediate-challenging
  * Time: medium
  * Prerequisites: C++
  * Related issues: [issue 130](https://code.google.com/p/texworks/issues/detail?id=130), [issue 371](https://code.google.com/p/texworks/issues/detail?id=371)

### Dynamically callabel scripts ###
Currently, scripts in TeXworks only support a very linear control flow: once they are started, they have to run to the end before control is returned to the main application. While this is useful for many simple or linear tasks, it keeps scripting from unfolding its full potential. The aim of this project is to implement the possibility for script functions to be called dynmically. On the one hand, this would enable scripts to call functions of other scripts, thereby opening the possibility to build and use "script function libraries". On the other hand, this would allow (with some hacking of Qt's meta-object-system) for scripts to respond to Qt signals, which in turn would open the road for script-based toolbar icons, symbol palettes, etc.

  * Difficulty: challenging
  * Time: long
  * Prerequisites: C++, Qt, QtScript/JavaScript, Lua
  * Related issues: [issue 317](https://code.google.com/p/texworks/issues/detail?id=317), [issue 369](https://code.google.com/p/texworks/issues/detail?id=369)