# Renderer Module

- `api` -> High-level public interface for interacting with the renderer.
- `backends` -> The implementation layer.

## Who defines the context?

The renderer declares the context, and the platform implements it.
This is because the context usually contains platform-specific code,
like SDL_GL_SwapBuffers(), that does not belong to the renderer.
