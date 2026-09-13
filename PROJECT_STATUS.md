# OllieEngine — Project Status

*Last updated: 13 September 2026 — committed and pushed as "Add component system, camera, and full MVP transform pipeline".*

This file is meant to be the fast-context doc for OllieEngine: what the engine currently does, how it's put together, and what was just changed — so a new chat (or a future you) can pick up the project without re-reading the whole codebase. Update it each time you commit: move today's "Latest Work" into the Commit Log, replace it with the new changes, and refresh the architecture snapshot if modules were added or restructured.

> **Note for Claude (or anyone helping with this project):** read this file first before answering questions about OllieEngine. It has the architecture, the most recent commit's changes, and where things stand — use it as the starting context instead of re-deriving it from scratch each session.

Each commit gets two kinds of questions below: **learning questions**, aimed at building up Ollie's understanding of C++ / engine programming / 3D graphics through the code just written (deliberately open — the point is to work them out, not read an answer); and **harder review questions**, which behave more like a code review and come with a possible solution attached, for the cases worth just knowing the answer to.

> **Question-writing preferences (Ollie's feedback, 10 Sept 2026) — apply to every future set:**
> - No OpenGL/graphics-API history or "deprecated feature" trivia (e.g. core vs. compatibility profile). Skip that entirely, not useful.
> - Mix in more questions about C++ itself — syntax/semantics, memory management (ownership, lifetimes, RAII, smart vs. raw pointers) — and abstract engine-architecture design, not just OpenGL/graphics mechanics.
> - Don't ask Ollie to reason about engine features that don't exist yet (e.g. "what would you need to add for X" when X isn't implemented). Note it as a roadmap/TODO item instead of turning it into a question.
> - Close each learning-question set with a small hands-on programming task rather than another conceptual question.

## What OllieEngine is

A small custom C++ game engine, built from scratch on top of GLFW (windowing/input), GLEW (OpenGL 3.3 core function loading), and GLM (vector/matrix math), using CMake. It's a learning/hobby engine project — currently a single "engine" static-ish library (`engine/`) consumed by a sample game executable (`source/`).

## Architecture snapshot

**`engine/source/`** — the engine library, namespace `eng`
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, the `GraphicsAPI`, and the active `Scene`. Each frame it also pulls `CameraData` (view + projection matrices) from the scene's main camera and passes it into `RenderQueue::Draw`, using the current window's aspect ratio.
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — polls keyboard state via GLFW callbacks.
- `graphics/GraphicsAPI` — wraps shader compilation and buffer creation.
- `graphics/ShaderProgram` — compiled shader program with cached uniform locations; `SetUniform` has float, float-pair, and `glm::mat4` overloads.
- `render/Material` — binds a `ShaderProgram` and holds float/float2 uniform params.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a data-defined `VertexLayout`.
- `render/RenderQueue` — collects `RenderCommand`s (material + mesh + modelMatrix). `Draw()` now also takes a `CameraData` (view + projection matrices) and sets `uModel`, `uView`, and `uProjection` on each command's shader before drawing.
- `scene/GameObject` — name, parent pointer, owned children, `IsAlive()`/`MarkForDestroy()` lifecycle, position/rotation/scale, `GetLocalTransform()`/`GetWorldTransform()` (recursively composed through the parent chain — the earlier `GetWorldTransfrom()` typo and its missing-return bug are both fixed now). **Also now owns a component list** `(new)`: `AddComponent(Component*)` and templated `GetComponent<T>()`, looked up by a per-type runtime id.
- `scene/Component` **(new)** — base class for per-object behaviour/data. `Component::StaticTypeId<T>()` hands out a stable id per type via a function-local static, and the `COMPONENT(Class)` macro generates the boilerplate `TypeId()`/`GetTypeId()` overrides. Holds a back-pointer (`m_owner`) to its `GameObject`, set by `AddComponent`.
- `scene/components/MeshComponent` **(new)** — owns a `Mesh` + `Material` pair; every `Update()` it submits a `RenderCommand` (using the owner's `GetWorldTransform()`) to the engine's `RenderQueue`.
- `scene/components/CameraComponent` **(new)** — computes a view matrix (`glm::inverse(owner's world transform)`) and a perspective projection matrix (`glm::perspective`, configurable FOV/near/far).
- `scene/Scene` — owns the root set of `GameObject`s. `CreateObject(name, parent)` / templated `CreateObject<T>(name, parent)`; `SetParent()` reparents with a cycle check. **Also now**: `SetMainCamera()`/`GetMainCamera()` and `Clear()`.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — implements `Application`; owns one `eng::Scene* m_scene`. `Init()` now also creates a camera `GameObject` with a `CameraComponent`, positions it, and calls `SetMainCamera()` on the scene.
- `TestObject` **(reworked)** — a `GameObject` subclass that no longer renders itself directly: it builds its shader/mesh/material in the constructor and hands them to a `MeshComponent` via `AddComponent()`. The vertex shader now does a full `uProjection * uView * uModel` transform (the engine's first real 3D pipeline) instead of the old model-only `uModel`. Still moves via WASD editing its inherited position, scaled by `deltaTime`.

**Third-party** (`engine/thirdparty/`): GLFW 3.4, GLEW, and GLM 1.0.1 — all vendored. `engine/CMakeLists.txt` was reworked this commit to use `target_include_directories(PUBLIC ...)` and link GLM as a proper `add_subdirectory` target (`glm`) via `target_link_libraries`, replacing the earlier bare `include_directories` approach. (The file still has the old pre-rework build script left in as commented-out dead code — harmless, but worth deleting in a cleanup pass.)

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable from `source/*` and links the `Engine` library, which pulls in GLFW/GLEW/GLM as described above.

## Latest work (committed 13 Sept 2026)

A component system, plus the engine's first real camera and full MVP transform pipeline:

- Added `Component` (base class, macro-based static type IDs via `COMPONENT(Class)`) and gave `GameObject` a component list: `AddComponent()` / templated `GetComponent<T>()`.
- Added `MeshComponent` (submits a `RenderCommand` each `Update`, using the owner's world transform) and `CameraComponent` (view matrix via inverse world transform; perspective projection matrix).
- Added `CameraData` (view + projection) to `RenderQueue::Draw`; it now sets `uView`/`uProjection` alongside `uModel` on every draw.
- Added `Scene::SetMainCamera()`/`GetMainCamera()`; `Engine::Run()` now pulls camera data from the scene's main camera every frame, using the live window aspect ratio.
- Fixed `GetWorldTransfrom()`'s typo (now `GetWorldTransform()`) **and** the missing `return` in its no-parent branch — closes harder-question #1 from the transform-hierarchy commit.
- Reworked `engine/CMakeLists.txt` to link GLM as a proper target and use `target_include_directories(PUBLIC ...)`.
- Rewired `TestObject` to own a `MeshComponent` instead of building render state directly; its vertex shader now does a full `uProjection * uView * uModel` transform.
- `Game::Init()` now creates a camera `GameObject` with a `CameraComponent` and sets it as the scene's main camera.

**Suggested commit message:**

```
Add component system, camera, and full MVP transform pipeline

- Add Component base class with macro-based static type IDs
  (COMPONENT), GameObject::AddComponent()/GetComponent<T>()
- Add MeshComponent (submits a RenderCommand each Update) and
  CameraComponent (view matrix via inverse world transform,
  perspective projection matrix)
- Add CameraData (view + projection) to RenderQueue::Draw; sets
  uView/uProjection alongside uModel each draw
- Add Scene::SetMainCamera()/GetMainCamera(); Engine::Run() pulls
  camera data from the scene's main camera each frame using the
  current window aspect ratio
- Fix GetWorldTransfrom() typo -> GetWorldTransform(), and fix the
  missing return in its no-parent branch (closes last commit's
  harder-question #1)
- Rework engine/CMakeLists.txt to link GLM as a proper target and
  use target_include_directories(PUBLIC ...)
- Rewire TestObject to own a MeshComponent instead of building
  render state directly; vertex shader now does a full
  uProjection * uView * uModel transform
- Game::Init() creates a camera GameObject with a CameraComponent
  and sets it as the scene's main camera
```

## Learning questions from this commit — C++, engine architecture & the component system

Grounded in the actual code from this commit (`Component`, `GameObject::AddComponent`, `CameraComponent`, `Game::Init`). Worth working through by hand rather than just reading an answer.

1. `AddComponent` is called like `AddComponent(new eng::MeshComponent(mesh, material));` — a raw `new`'d pointer, wrapped in a `unique_ptr` only once it's inside `emplace_back`. Why is accepting a `std::unique_ptr<Component>` parameter directly usually considered safer than this "raw pointer in, wrapped internally" pattern? What's the concrete way the current signature could leak?
2. `Component::StaticTypeId<T>()` hands out a per-type id using `static size_t typeId = nextId++;` inside a function template. Why does one function-local static per template instantiation guarantee a stable, unique id for each component type — and what would break if `nextId` were an ordinary member variable instead of a function-local static?
3. The codebase now has three places holding a *raw, non-owning* pointer to something owned elsewhere: `GameObject::m_parent`, `Game::m_scene` (the real owner is `Engine::m_currentScene`, a `unique_ptr`), and the local `camera` variable in `Game::Init()` (owned by the `Scene`). What's the one rule all three have to follow to stay safe, and what would go wrong if something used one of them after its real owner had already destroyed it?
4. `CameraComponent::GetViewMatrix()` returns `glm::inverse(m_owner->GetWorldTransform())` rather than the world transform itself. In plain terms, why does "where the camera is in the world" need to be inverted to produce "how the world looks from the camera"?
5. **Programming task:** add a `RemoveComponent<T>()` method to `GameObject` (mirroring `GetComponent<T>()`'s type-id lookup) that finds and erases a component of type `T` from `m_components`. Then use it somewhere in `TestObject` to prove it works — e.g. toggle the `MeshComponent` off and on with a key so the quad disappears and reappears.

## A few harder questions (with possible solutions)

More "code review" than "learning exercise" — real gaps or bugs, each with one reasonable way to close it.

1. **Still no depth testing — and now it actually matters.** With a real camera and perspective projection in place, the moment a second 3D object exists (or this one rotates edge-on to another), submission order — not depth — decides what's drawn on top. Nothing in `GraphicsAPI` enables depth testing or clears a depth buffer.
   *Possible solution:* `glEnable(GL_DEPTH_TEST)` once during setup, and clear `GL_DEPTH_BUFFER_BIT` alongside the color buffer in `ClearBuffers()`.

2. **Direct uniform-pushing around `Material` has grown, not shrunk.** `RenderQueue::Draw` now sets *three* uniforms (`uModel`, `uView`, `uProjection`) straight on the shader program, still completely bypassing `Material::SetParam`/`Bind()` — which was flagged for just `uModel` last commit and hasn't been addressed since; there are now two more uniforms doing the same workaround.
   *Possible solution:* unchanged from last time — give `Material` a `glm::mat4` overload for `SetParam`, and route all per-draw uniforms (model, view, projection) through `Material::Bind()` instead of reaching into the shader directly from `RenderQueue`.

3. **Index buffer still uploads the wrong type's byte size** *(open three commits running)* — `GraphicsAPI::CreateIndexBuffer` still computes `indices.size() * sizeof(float)` for a `std::vector<uint32_t>`.
   *Possible solution:* unchanged — `indices.size() * sizeof(uint32_t)` (or `sizeof(indices[0])`).

4. **Shader-compile failure still isn't checked** *(open three commits running)* — component-based or not, nothing checks `CreateShaderProgram`'s result for null before handing it to a `Material`.
   *Possible solution:* unchanged — check the returned `shared_ptr` for null and log + bail (or fall back to an "error" shader) before continuing.

## Commit log

| Date | Summary |
|---|---|
| 2026-08-21 | Initial commit — engine core: `Engine` singleton (GLFW window/context, game loop), `Application` interface, `InputManager`, `GraphicsAPI`, `ShaderProgram`, `Material`, `Mesh`, vendored GLFW 3.4 + GLEW, sample triangle-rendering `Game`. |
| 2026-09-10 | Scene/GameObject hierarchy + `TestObject` sample — added `GameObject` (parent/child ownership, `IsAlive()`/`MarkForDestroy()`, cascading `Update()`), `Scene` (object storage, templated `CreateObject<T>()`, `SetParent()` with cycle checking), and `TestObject` as the first real `GameObject` (own shader/material/mesh, WASD-driven `uOffset`, submits to `RenderQueue`). |
| 2026-09-10 | Transform hierarchy via GLM — `GameObject` gained position/rotation/scale plus `GetLocalTransform()`/`GetWorldTransfrom()`; `ShaderProgram`, `RenderQueue`, and `TestObject` rewired to build and submit a `uModel` matrix instead of the old `uOffset` uniform (also fixed the frame-rate-dependent movement flagged last time). |
| 2026-09-13 | Component system + camera + full MVP pipeline — added `Component`/`MeshComponent`/`CameraComponent`; `RenderQueue`/`Engine` now build real `uView`/`uProjection` matrices from the scene's main camera each frame; fixed the `GetWorldTransfrom()` typo and its missing-return bug; reworked `engine/CMakeLists.txt`'s GLM linking. |
