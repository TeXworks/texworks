# Using language services

Language services are optional and disabled by default. They add assistance
from a separately configured external provider without replacing TeXworks'
normal editor or typesetting workflow. TeXworks does not include a language
service provider.

TeXworks exposes a provider-neutral language-service feature. Its current
adapter implements a limited subset of the Language Server Protocol (LSP), so
it does not promise compatibility with every LSP server. [Digestif](https://github.com/astoff/digestif)
is a tested example for ConTeXt/LMTX; it is not required to use TeXworks.

## Configure a language service

Open **Preferences**, select **General**, and find **Language Services**.

1. Select **Enable language services**.
2. Choose the language-server executable with **Browse...** or enter its path.
3. Enter its arguments with one literal argument on each line. TeXworks does
   not split a line as a shell would, so quoting or combining several
   arguments on one line does not create separate arguments.
4. Click **OK** to save the preferences. The status text reports whether the
   server is starting, available, stopped, or unavailable.

Each line is passed unchanged as one argument: spaces, quotes, and backslashes
are literal characters, and a blank line represents an empty argument. No
shell parsing or expansion is performed.

TeXworks does not find or install a provider automatically. The
`TEXWORKS_TEST_LANGUAGE_SERVER` and `TEXWORKS_LANGUAGE_SERVER` environment
variables are for developer tests, not for ordinary configuration.

## ConTeXt documents

Modern ConTeXt files with `.mkiv` and `.mkxl` extensions are identified as
ConTeXt sources. For a stored `.tex` document, a program modeline takes
precedence: it is identified as ConTeXt when that modeline names ConTeXt
(`context`). A different program modeline is not overridden by selecting a
ConTeXt engine. Only when no program modeline is present can a selected
ConTeXt engine identify the document as ConTeXt. The default **ConTeXt
(LuaMetaTeX)** engine remains distinct from **ConTeXt (LuaTeX)**.

Language-service assistance is currently configured for these ConTeXt sources;
it does not imply support for other TeX dialects.

## Completion

TeXworks' built-in static completion remains available whether or not a
language service is configured. When the active service advertises completion,
provider candidates can be added asynchronously to the normal completion
popup. Keyboard navigation, acceptance, and cancellation use the usual editor
completion behavior.

## Go to Definition

**Go to Definition** is available when the active service advertises and
supports definitions for the current synchronized document. A single returned
usable local-file location is opened and its returned range is selected. If
the sole returned location cannot be opened or navigated, TeXworks reports
that the definition location cannot be opened. If no location, or more than
one location, is returned, TeXworks reports that result instead of choosing
one.

## If the service is unavailable

With no service configured, an invalid executable, a failed service, or after
disabling the feature, provider assistance is unavailable. TeXworks keeps
ordinary editing, typesetting, and static completion independent of the
service. Correct the Preferences configuration or enable the feature again to
start a new service.

## Current limits

This feature does not currently provide a diagnostics UI, arbitrary workspace
semantics, automatic provider discovery, automatic restart, generic
server-request features, or every LSP capability.
