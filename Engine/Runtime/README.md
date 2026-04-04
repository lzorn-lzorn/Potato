# How to correctly add submodules

The Game module links to the engine module, which by default is the Runtime module. The engine also expects the Game module to link only to the Runtime module.

The top‑level directories under Runtime are themselves submodules, and they also act as module namespaces (i.e., they contain further submodules).  
To import a module in Runtime, simply `add_subdirectory` the corresponding folder.

How should internal submodules (e.g., adding module A inside the Core module) be added?  
Assume the directory structure of A is as follows:  
Public headers are placed directly under the root directory of A, so that in Game you can write `#include "Core/A/A.h"`.

Use the `add_internal_module` function to add the corresponding library information:

```cmake
# A/CMakeLists.txt:
# Main function: add an internal module
# Call signature:
#   add_internal_module(<name>
#       TYPE      <STATIC|SHARED|OBJECT|INTERFACE>   # optional, default is STATIC
#       SOURCES   <src1> [<src2> ...]                # list of source files
#       INCLUDES  <inc1> [<inc2> ...]                # list of include directories (public)
#       DEPENDENCY <dep1> [<dep2> ...]               # optional, other targets this module depends on
#       PRIVATE_INCLUDES  <inc1> [<inc2> ...]        # list of private include directories, e.g. SDL3::SDL3
#   )

The INCLUDES parameter should be a public header directory – typically ${CMAKE_CURRENT_SOURCE_DIR}.
For private headers, use either:

```CMake
target_include_directories(MyModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivateInclude
    ${CMAKE_CURRENT_SOURCE_DIR}/Source   # if private headers are mixed with sources
)
```

or pass them via the PRIVATE_INCLUDES argument of add_internal_module.