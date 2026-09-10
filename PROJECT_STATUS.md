# OllieEngine — Project Status

*Last updated: 10 September 2026 — committed and pushed as "Add Scene/GameObject hierarchy and TestObject render sample".*

This file is meant to be the fast-context doc for OllieEngine: what the engine currently does, how it's put together, and what was just changed — so a new chat (or a future you) can pick up the project without re-reading the whole codebase. Update it each time you commit: move today's "Latest Work" into the Commit Log, replace it with the new changes, and refresh the architecture snapshot if modules were added or restructured.

## What OllieEngine is

A small custom C++ game engine, built from scratch on top of GLFW (windowing/input) and GLEW (OpenGL 3.3 core function loading), using CMake. It's a learning/hobby engine project — currently a single "engine" static-ish library (`engine/`) consumed by a sample game executable (`source/`).

## Architecture snapshot

**`engine/source/`** — the engine library, namespace `eng`
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, and the `GraphicsAPI`. Also now owns/drives the active `Scene` implicitly through the `Application`.
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — polls keyboard state via GLFW callbacks.
- `graphics/GraphicsAPI` — wraps shader compilation and buffer creation.
- `graphics/ShaderProgram` — compiled shader program with cached uniform locations.
- `render/Material` — binds a `ShaderProgram` and holds float uniform params.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a data-defined `VertexLayout`.
- `render/RenderQueue` — collects `RenderCommand`s (material + mesh) submitted during `Update` for drawing.
- `scene/GameObject` **(new)** — base class for anything living in the scene: name, parent pointer, owned children, `IsAlive()` / `MarkForDestroy()` lifecycle, and a virtual `Update()` that cascades to children and prunes dead ones.
- `scene/Scene` **(new)** — owns the root set of `GameObject`s. `CreateObject(name, parent)` makes a plain `GameObject`; the templated `CreateObject<T>(name, parent)` makes any `GameObject` subclass. `SetParent()` handles reparenting an object between the scene root and another object's children, including a walk-up-the-chain check to reject creating a cycle.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — implements `Application`; owns one `eng::Scene` (`m_scene`).
- `TestObject` **(new)** — a `GameObject` subclass: builds an inline shader (position + vertex color, with a `uOffset` uniform), a colored quad `Mesh`, moves itself with WASD by editing `uOffset` each frame, and submits itself to the `RenderQueue`.

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable from `source/*`, pulls in `engine/` as a subdirectory (which vendors GLFW 3.4 and GLEW under `engine/thirdparty/`), and links the `Engine` library.

## Latest work (committed 10 Sept 2026)

Scene graph plus a working example object built on it:

- Added `GameObject`: parent/child ownership, `IsAlive()` / `MarkForDestroy()` for deferred removal, and an `Update()` that recurses into children and erases dead ones.
- Added `Scene`: object storage at the root, a templated `CreateObject<T>()` on top of the existing untyped `CreateObject()`, and `SetParent()` for moving objects between the root and other objects' child lists (with a cycle check when reparenting under a live object).
- Added `TestObject`, the first real `GameObject`: constructs its own shader/material/mesh (a colored quad) in its constructor, reads WASD from the `InputManager` to drive a `uOffset` shader uniform, and submits a `RenderCommand` every frame.
- Rewired `Game`: `Init()` now spawns a `TestObject` via `m_scene.CreateObject<TestObject>("TestObject")` instead of building geometry directly, and `Update()` just calls `m_scene.Update(deltaTime)`.

**Suggested commit message:**

```
Add Scene/GameObject hierarchy and TestObject render sample

- Add GameObject: parent/child ownership, IsAlive()/MarkForDestroy()
  for deferred removal, Update() cascades into children and prunes
  dead ones
- Add Scene: object storage, templated CreateObject<T>() alongside
  the existing untyped CreateObject(), and SetParent() for
  reparenting objects (with cycle checking)
- Add TestObject: builds its own shader/material/colored-quad mesh,
  moves via WASD by driving a uOffset uniform, submits to the
  RenderQueue each frame
- Rewire Game to spawn a TestObject through the new scene graph
  instead of building render geometry directly
```

## Ten questions worth answering before/while building on this

1. `CreateObject`/`CreateObject<T>` return a raw `GameObject*`. Once `MarkForDestroy()` fires and the object is erased next `Update()`, any raw pointer a caller kept becomes dangling — do you want a handle/ID-based reference instead before more code starts holding onto these pointers?
2. `GameObject`'s default constructor is protected and `Scene::CreateObject<T>()` always calls `new T()` with no arguments — how should subclasses that need constructor parameters (initial position, config, etc.) be created, since `TestObject` currently side-steps this by doing all its setup in a no-arg constructor?
3. `MarkForDestroy()` just flips a flag; actual removal happens on the *next* `Update()` pass of whichever container (`Scene` or the parent `GameObject`) owns it. Is that one-frame-lag deletion timing intentional, and should there be an `OnDestroy()` hook for cleanup (e.g. releasing GL resources) before the object is erased?
4. `Scene::SetParent` walks up from the new parent to check for cycles when reparenting under a live parent — does that same check need to run in the "reparenting to root" branches too, or is a cycle impossible there by construction?
5. `TestObject` hardcodes its shader source, vertex/index data, and WASD bindings directly in the constructor — is that meant to stay a throwaway example, or is extracting shaders to asset files and input to a bindable action layer the next step?
6. There's no way yet to look up a `GameObject` by name or type from `Scene` (or from a sibling `GameObject`) — is that needed soon, or is holding onto the pointer `CreateObject` returns enough for now?
7. `TestObject` fakes position with a raw `uOffset` shader uniform instead of any shared notion of a transform. Now that a parent/child hierarchy exists, is a `Transform` component (with parent-relative position/rotation/scale) the natural next addition?
8. Rendering currently happens by each `GameObject` calling `RenderQueue.Submit()` itself from inside `Update()`. As more objects render, do you want a separate `Render()`/`Draw()` virtual (distinct from `Update()`) so `Scene` can control draw ordering, culling, or visibility independently of update logic?
9. `main.cpp` hardcodes the window to 1280×720 with no title/vsync/config options — worth moving to a small config file or command-line args now, or fine to leave hardcoded while the engine is still this early?
10. Given how easy it'd be to get a dangling `GameObject*` from the lifecycle above, do you want at least a couple of manual test scenes (or real unit tests) around `Scene`/`GameObject` reparenting and destruction before more gameplay code starts depending on it?

## Commit log

| Date | Summary |
|---|---|
| 2026-08-21 | Initial commit — engine core: `Engine` singleton (GLFW window/context, game loop), `Application` interface, `InputManager`, `GraphicsAPI`, `ShaderProgram`, `Material`, `Mesh`, vendored GLFW 3.4 + GLEW, sample triangle-rendering `Game`. |
| 2026-09-10 | Scene/GameObject hierarchy + `TestObject` sample — see "Latest work" above. |
