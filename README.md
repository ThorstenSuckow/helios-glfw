# helios::glfw

GLFW platform integration for the helios engine modules.

## Overview

`helios::glfw` provides a concrete GLFW-backed implementation for platform
initialization, native window lifecycle, resize propagation, buffer swapping,
and close-request handling.

## Features

- GLFW runtime initialization and shutdown handling
- Native window creation from engine window commands
- Framebuffer/window resize propagation
- Buffer swap command processing
- Typed GLFW callback user-pointer bridge
- Window-close polling system

## Module surface

| Area | Public modules / APIs |
|------|------------------------|
| Platform manager | `GLFWPlatformManager` |
| Components | `GLFWWindowHandleComponent`, `GLFWWindowUserPointerComponent` |
| Systems | `GLFWWindowCloseSystem` |
| Types | `helios.glfw.types` |
| Aggregator | `helios.glfw` |

## Usage

### C++ module

```cpp
import helios.glfw;
```

### Platform architecture

`GLFWPlatformManager<TRenderPlatform, THandle, TStateCommandBuffer, TPlatformCommandBuffer>`
receives platform/window commands, stores pending work, and applies it in
`flush(UpdateContext&)`.

GLFW-native data is attached to window entities through ECS components:

- `GLFWWindowHandleComponent<THandle>` stores the native `GLFWwindow*`
- `GLFWWindowUserPointerComponent<THandle, TPlatformCommandBuffer>` stores typed callback payload

`GLFWWindowCloseSystem<THandle, TCommandBuffer>` scans shown active windows and
queues `WindowCloseCommand<THandle>` when GLFW reports a close request.

### CMake

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

Configure a consumer against an installed prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/helios-prefix"
cmake --build build
```

## Development

Run the regular CMake build from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

## Related repositories

- [`helios-ecs`](https://github.com/thorstensuckow/helios-ecs)
- [`helios-engine`](https://github.com/thorstensuckow/helios-engine)
- [`helios-math`](https://github.com/thorstensuckow/helios-math)
- [`helios-opengl`](https://github.com/thorstensuckow/helios-opengl)
