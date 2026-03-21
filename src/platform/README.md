# Platform Module (Entrypoint)

- `api` -> High-level platform abstraction layer.
- `backends` -> The implementation layer.

The platform layer is split into various "servers".
Each server its own role. For example, the display server 
defines how a window should be created, and the OS server
exposes polling functions like the OS name.

## What is, and what's not, the platform's responsibility?

The platform:
- Defines the entrypoint
- Owns the engine.
- Provides a thin layer for low-level OS/platform code.
  - This exposes platform functions for the engine,
    that allows it to perform window management and
    I/O operations, for example.

It does not:
- Define how the engine should operate.
- Create a render device and draw stuff.
  - With the exception of providing graphics context 
    implementation. It does not create it, however.
