# Graphics Module

Contains the backend implementations of each render device.

- `api` -> High-level public interface for interacting with a render-device.
- `backends` -> The implementation of the render device interface.

## What's a render device?

A render device is a **Rendering Abstraction Layer (RAL)** that exposes somewhat
low- to mid-level commands for interacting with the GPU through a graphical API.

It provides operations such as buffer allocation, pipeline creation, indexed drawing,
matrix manipulation, texture uploading, and more.

Its primary objective is to allow the renderer to work across a variety of
platforms and graphics backends with minimal changes.

## Who owns render device creation?

The platform module is responsible for selecting and linking the appropriate
render device backend. It provides a render device factory, which is then passed
to the runtime renderer. The renderer uses this factory to create the render device.

## Who defines the context?

Some render devices, such as OpenGL, require an additional context layer to interact
with a window. In this case, the render device defines a context interface that
the platform must implement.
