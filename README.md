# helios::platform::glfw

GLFW-backed platform integration.

## Overview

This module provides the concrete GLFW implementation used by the platform
layer, including a manager plus GLFW-specific bridge components/types/systems.

## Key Types

| Type | Purpose |
|------|---------|
| `GLFWPlatformManager<THandle>` | Concrete manager handling GLFW init/window/event integration |

## Subdirectories

| Directory | Purpose |
|-----------|---------|
| `components/` | GLFW-native handles and user pointer bridge components |
| `systems/` | GLFW-specific window-close integration systems |
| `types/` | GLFW wrapper value types |


## CMake package usage

Build and install:

```bash
cmake -S . -B build -DHELIOS_GLFW_ENABLE_PACKAGE=ON -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build
cmake --install build
```

Consume from another project:

```cmake
find_package(helios-engine CONFIG REQUIRED)
find_package(helios-glfw CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE helios::glfw)
```


---
<details>
<summary>Doxygen</summary><p>
@namespace helios::platform::glfw
@brief GLFW-backed platform integration.
</p></details>
