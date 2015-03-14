# Using code completion #

TeXworks comes with an expandable set of code templates that can easily be inserted into a (La)TeX document. To insert a code template, type the first few characters of the pattern name and press `<Tab>`. The code template best matching the characters you typed will be inserted and highlighted. If there are multiple code templates starting with the characters you typed press `<Tab>` to cycle through the possibilities.

Some code templates include bullet characters •. These act as placeholders where text should be inserted. Use `<Ctrl>+<Tab>` and `<Shift>+<Ctrl>+<Tab>` (`<Alt>+<Tab>` and `<Alt>+<Shift>+<Tab>` on Mac) to navigate between those placeholders.

Some of the most common code templates are listed below:

| --   |  \textendash |
|:-----|:-------------|
| ---  |  \textemdash |
| lbl  |  \label{} |
| ref  |  \ref{} |
| sec  |  \section{} |
| ssec |  \subsection{} |
| bf   |  \textbf{} |
| em   |  \emph{} |
| xa   |  \alpha |
| xb   |  \beta |
| _...etc..._  |

# Customizing code completion #

The code completion templates are located in the `completion` sub-directory of your [TeXworks resource directory](TipsAndTricks#Locating_and_customizing_TeXworks_resources.md).

The general syntax of a code completion template is the following:
```
	<alias>:=<text>
```
The `<alias>:=` part can be omitted to turn the code text into its own alias. `<text>` must fit in a single line (see below for ways to produce a multi-line code template). Empty lines and lines starting with a `%` are ignored.

In `<text>` several special markers are allowed:

| `#RET#` | is replaced by a line break upon expansion of the template |
|:--------|:-----------------------------------------------------------|
| `#INS#` | upon expansion the cursor is placed at this position. Any additional #INS# markers are ignored |
| `•`     | the Unicode character U+2022 (BULLET) is used as a placeholder where text should be entered; navigate through to the placeholders with `<Ctrl>+<Tab>` and `<Shift>+<Ctrl>+<Tab>` (`<Alt>+<Tab>` and `<Alt>+<Shift>+<Tab>` on Mac) |

Please refer to the existing code completion template files for numerous examples.

_Thanks to Stefan Löffler for this writeup._