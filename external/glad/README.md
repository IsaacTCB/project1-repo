# glad

Multi-Language Vulkan/GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs,
created and maintained by _Dav1dde_ & the _glad_ contributors

Source code can be found in glad's [official repository](https://github.com/Dav1dde/glad/tree/glad2).

_glad2_ is used to load _OpenGL 3.3_ using _Core_ profile for the OpenGL renderer.

## Generated files

The following files were generated using [glad2's webservice generator](https://gen.glad.sh):

- `include/KHR/khrplatform.h`
- `include/glad/gl.h`
- `src/glad/gl.c`

### Generator Settings

To produce those exact same files:

- **gl:** Version 3.3 - Core
- **extensions**: `GL_KHR_debug`

Everything else should remain untouched.

## Files extracted from upstream

- `LICENSE`

## Changes from upstream

- Added `CMakeLists.txt` file.
