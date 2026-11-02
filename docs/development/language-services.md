# Language-service architecture

This document records the current provider-neutral language-service contract.
It describes the implemented seam; it is not a generic editor-plugin design.

## Ownership and lifecycle

### `LanguageService`

`LanguageService` is the provider-neutral capability and lifecycle interface.
It owns the contract for starting and stopping a service, document
synchronization operations, capability reporting, and completion and definition
requests/results. Provider-specific protocol, process, and UI policy do not
belong in this interface.

### `LanguageServiceManager`

`LanguageServiceManager` owns active and pending service replacement, lifecycle
coordination, registered documents, and creation/removal of document bindings.
It is the authority for associating registered documents with a service.

`TWApp` coordinates persisted configuration: it reads and writes the
Preferences settings and applies a configured service. It does not own the
language-service lifecycle or document-binding authority.

### `LanguageServiceDocumentBinding`

Each binding owns one document's service identity and synchronization. This
includes open/change/close handling, document versions, service and identity
generations, freshness checks, request tokens, and capability-gated completion
and definition operations. It also performs UTF-16 position conversion where
the protocol operation requires positions.

Results must remain fresh for the same service generation, document identity,
and synchronized version before reaching the UI. This prevents delayed results
from a changed, renamed, closed, or replaced document from being presented.

## LSP boundary

`LspLanguageService` is the provider/protocol adapter and policy layer for the
implemented LSP subset. It maps advertised server capabilities and only enables
operations that TeXworks implements.

`LanguageServerProcess` owns child-process input/output and shutdown handling.
`JsonRpcTransport` owns generic JSON-RPC framing and request/response
correlation. Notifications and requests remain distinct. Valid but unsupported
server-to-client requests receive a provider-neutral JSON-RPC Method-not-found
rejection; this does not turn server-request features into implemented product
features.

Protocol, transport, and process authority must not move into `CompletingEdit`,
`TeXDocumentWindow`, `TeXDocument`, syntax loaders, or other UI/document code.

## UI boundary

`CompletingEdit` and `TeXDocumentWindow` consume capability-gated completion
and definition results. They do not decide provider protocol or process policy.
Static completion remains independent of language-service availability.

## Adding another provider

A future provider should implement the `LanguageService` seam and keep its
protocol and process concerns behind that boundary. Wire configuration through
the existing application-level coordination as needed, while leaving document
binding authority with `LanguageServiceManager` and presentation in the UI.
Do not add provider-specific JSON-RPC or process logic to editor, window, or
document classes. No plugin architecture is implied by this contract.

## Compatibility contract

The README declares the existing policy floor: CMake 3.1.0, Qt 5.2.3, poppler
0.24.5, and hunspell 1.2.9. The established clean Trusty proof environment
resolved Qt 5.2.1 with CMake 3.1.0 and GCC 4.8.4. That older proof is
compatibility evidence; it does not change the declared Qt 5.2.3 policy floor.

## Optional real-provider validation

These variables are for developers and tests only. Ordinary users configure a
provider in Preferences and do not use either variable.

- `TEXWORKS_TEST_LANGUAGE_SERVER` enables direct adapter/session validation in
  `LanguageServiceSessionTest`.
- `TEXWORKS_LANGUAGE_SERVER` enables production activation/window validation
  in `LanguageServiceNavigationWindowTest`.

For example:

```sh
export TEXWORKS_TEST_LANGUAGE_SERVER=/path/to/language-server
export TEXWORKS_LANGUAGE_SERVER=/path/to/language-server
```

Setting only one deliberately exercises only its corresponding layer; the
other optional real-provider test is skipped. Set both, with executable paths
appropriate for the relevant test, to obtain both kinds of optional
real-provider evidence.
