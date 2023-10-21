# Small notes about the TWX build and test system

This is a work in progress.

There are many various scripts here, each one is dedicated to a rather simple situation.

## Facts from CMake documentation, or not

Here are collected some documentation extracts useful for this build system.
In the sequel, `?` stands for either `SOURCE` or `BINARY`.

### -P script mode and cache.

The documentation states in https://cmake.org/cmake/help/latest/manual/cmake.1.html#run-a-script:
"No configure or generate step is performed and the cache is not modified."
It means that
* if there is no `CMakeCache.txt`, none is created,
* if there is a `CMakeCache.txt`, it is not modified.
What we also must understand is that if there is a `CMakeCache.txt`,
it is ignored.
So using `CMakeCache.txt` is not suitable to share information with
scripts of custom targets.

### set
Undocumented behavior(?): setting a variable in the parent scope does not change the eponym local variable
```
set ( A BEFORE )
function ( test )
  set ( A AFTER PARENT_SCOPE )
  message ( "${A}" )
endfunction ()
message ( "${A}" )
```
prints
```
BEFORE
AFTER
```
This is like `set ( ... PARENT_SCOPE )` were postponed to the end of the current scope or the parent variables were copied before entering the child scope.

### CMakeLists.txt and projects
* Primary `CMakeLists.txt`:
  - `CMAKE_?_DIR`, as well as `CMAKE_CURRENT_?_DIR` are properly set
  - `CMAKE_?_DIR` won't change (normally)
  - `CMAKE_CURRENT_LIST_DIR` value is `CMAKE_SOURCE_DIR`'s as well as
    `CMAKE_CURENT_SOURCE_DIR`'.
  - Any `project(<PROJECT-NAME>)` instruction:
    * set `CMAKE_PROJECT_NAME` and `PROJECT_NAME` to `<PROJECT-NAME>`,
    * set variables `PROJECT_?_DIR` and `<PROJECT-NAME>_?_DIR` to
      `CMAKE_CURRENT_?_DIR`.
  - On subsequent starts `CMAKE_PROJECT_NAME` may be set
    since the very beginning but its value is not reliable.
  - `add_subdirectory( <subdir> )`: loads and executes the secondary
    `CMakeLists.txt` in `${CMAKE_CURRENT_SOURCE_DIR}/<subdir>`.
    Once complete continue execution of the primary `CMakeLists.txt`.
* Secondary CMakeLists.txt:
  - `${CMAKE_CURRENT_?_DIR}/<subdir>` become the new values of
    `CMAKE_CURRENT_?_DIR`. When the control comes back to the callee,
    at the end of the script, these variables take their previous value
    (`add_subdirectory` declares a new scope).
  - `CMAKE_CURRENT_LIST_DIR`'s value is `CMAKE_CURENT_SOURCE_DIR`'.
  - Any `project(<PROJECT-NAME>)` instruction:
    * set `PROJECT_NAME` to `<PROJECT-NAME>`, but `CMAKE_PROJECT_NAME`
      is left untouched.
    * set variables `PROJECT_?_DIR` and `<PROJECT-NAME>_?_DIR` to
      `CMAKE_CURRENT_?_DIR`.

From a different POV:

* `PROJECT_NAME`: Name of the project given to the project command.
  This is the name given to the most recent `project()` command.
* `CMAKE_PROJECT_NAME`: The name of the current project. This specifies
  name of the current project from the closest inherited `project` command.
  See: https://stackoverflow.com/questions/38938315/difference-between-cmake-project-name-and-project-name
  At the beginning of a `CMakeLists.txt` loaded through `add_subdirectory`,
  both are the same, but after a `project()` they become different.
* `CMAKE_CURRENT_SOURCE_DIR`/`CMAKE_CURRENT_BINARY_DIR`: this the full path
  to the source directory that is currently being processed by cmake.
  Each directory added by add_subdirectory will create a binary directory
  in the build tree, and as it is being processed this variable will be
  set.
* `CMAKE_SOURCE_DIR`/`CMAKE_BINARY_DIR`: the path to the top level of
  the source/binary tree. This is the full path to the top level of the
  current CMake source/binary tree.
* `PROJECT_SOURCE_DIR`/`PROJECT_BINARY_DIR`: top level source/binary
  directory for the current project. This is the source/binary directory
  of the most recent `project()` command.
* `<PROJECT-NAME>_SOURCE_DIR`, `<PROJECT-NAME>_BINARY_DIR`: Top level source and binary directory for the named project.
These variables are created with the name used in the `project()` command.
This can be useful when `add_subdirectory()` is used to connect several projects.

NB: It allows the `CMakeLists.txt` in the subdirectory access to the outer
source and binary directory when it defines its own `project()`.
However, it assumes that the outer project name is known
by the inner `CMakeLists.txt` or at least a variable name is dedicated.

* CMake `-P`: Process script mode.
  Process the given cmake file as a script written in the CMake language.
  No configure or generate step is performed and the cache is not modified.
  If variables are defined using `-D`, this must be done before the `-P` argument.

* NB: For an in-source build, the `*_BINARY_*` and `*_SOURCE_*` are always the same. We do not assume nor recommand in-source building.

## Design explanations

We always start with a top level `CMakeLists.txt` which we call primary.
This one starts with
```
cmake_minimum_required ( ... )
...
project( <ROOT-PROJECT-NAME> )
``` 
At this point `PROJECT_NAME` and `CMAKE_PROJECT_NAME` both contain
`<ROOT-PROJECT-NAME>`.
The various `CMAKE_CURRENT_?_DIR`, `CMAKE_?_DIR`, `PROJECT_?_DIR`
and `<PROJECT-NAME>_?_DIR` are all the same.

## More complete test

Test folder from which we run `cmake .`
```
|-CMakeLists.txt
\-b
  |-CMakeLists.txt
  \-c
    \-CMakeLists.txt
```
Primary `CMakeLists.txt`:
```
cmake_minimum_required(VERSION 3.0)
function (test prefix)
  file(
    RELATIVE_PATH
    CMAKE_CURRENT_SOURCE_DIR
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if (PROJECT_SOURCE_DIR)
    file(
      RELATIVE_PATH
      PROJECT_SOURCE_DIR
      "${CMAKE_SOURCE_DIR}"
      "${PROJECT_SOURCE_DIR}"
    )
  else ()
    Set ( PROJECT_SOURCE_DIR "?" )
  endif ()
  file(
    RELATIVE_PATH
    CMAKE_CURRENT_BINARY_DIR
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
  )
  If (PROJECT_BINARY_DIR)
    file(
      RELATIVE_PATH
      PROJECT_BINARY_DIR
      "${CMAKE_BINARY_DIR}"
      "${PROJECT_BINARY_DIR}"
    )
  Else ()
    Set ( PROJECT_BINARY_DIR "?" )
  Endif ()
  if ( NOT PROJECT_NAME )
    set ( PROJECT_NAME "  " )
  endif ()
  message(
    "${prefix}${CMAKE_PROJECT_NAME}|${PROJECT_NAME}|∑/${CMAKE_CURRENT_SOURCE_DIR}|∑/${PROJECT_SOURCE_DIR}|ß/${CMAKE_CURRENT_BINARY_DIR}|ß/${PROJECT_BINARY_DIR}|"
  )
endfunction ()
Test ( "<   " )
project(A1)
Test ( "<   " )
project(A2)
Test ( "<   " )
add_subdirectory(b)
Test ( "<   " )
project(A3)
Test ( "<   " )
```
Secondary `b/CMakeLists.txt`:
```
Test ( "<<  " )
project(B1)
Test ( "<<  " )
add_subdirectory(test3)
Test ( "<<  " )
project(B2)
Test ( "<<  " )
```
Secondary `c/CMakeLists.txt`:
```
Test ( "<<< " )
project(C1)
Test ( "<<< " )
```
The relevant output is: `CMAKE_PROJECT_NAME`|`PROJECT_NAME`|`CMAKE_SOURCE_DIR`|`CMAKE_CURRENT_SOURCE_DIR`|`PROJECT_SOURCE_DIR`|`CMAKE_BINARY_DIR`|`CMAKE_CURRENT_BINARY_DIR`|`PROJECT_BINARY_DIR`|
```
<   A3||∑/|∑/?|ß/|ß/?|
<   A1|A1|∑/|∑/|ß/|ß/|
<   A2|A2|∑/|∑/|ß/|ß/|
<<  A2|A2|∑/b|∑/|ß/b|ß/|
<<  A2|B1|∑/b|∑/b|ß/b|ß/b|
<<< A2|B1|∑/b/c|∑/b|ß/b/c|ß/b|
<<< A2|C1|∑/b/c|∑/b/c|ß/b/c|ß/b/c|
<<  A2|B1|∑/b|∑/b|ß/b|ß/b|
<<  A2|B2|∑/b|∑/b|ß/b|ß/b|
<   A2|A2|∑/|∑/|ß/|ß/|
<   A3|A3|∑/|∑/|ß/|ß/|
```
Assuming normal behaviour (no weird things)
* `CMAKE_PROJECT_NAME` only changes at each `project()` instructions
  of the primary `CMakeLists.txt`.
* `CMAKE_?_DIR` is only set at the top level
* `CMAKE_CURRENT_?_DIR` is set by each `add_subdirectory`.
* `CMAKE_CURRENT_?_DIR` equals `PROJECT_?_DIR` after each `project()`.
* There is no stacking of project structure.
* If `PROJECT_NAME` is set, a `project()` instruction has been issued before
  in particular, a `CMakeLists.txt` can guess/expect it is secondary.

## TeXworks specials

We want to test only parts of the project.
Then we need to rely on modules that can be managed independently or
as part of a bigger project.
The modules need to share information and functionalities that superseed
any `CMakeLists.txt`, whether primary or secundary.
Moreover, some targets run in process mode may need to access these
information and functionalities as well.
For example the git information is always tied to the top level directory,
which is not always `CMAKE_SOURCE_DIR`

## Tips and tricks

* `CMAKE_SCRIPT_MODE_FILE` is not void only when the script is launched in script mode.
